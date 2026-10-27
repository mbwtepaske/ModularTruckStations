"""Builds the cargo module BPs, descriptors, recipes and unlock schematic from Tools/module_columns.yaml.

Runs inside the Unreal editor (Tools > Execute Python Script), or headless with the editor closed:
    UnrealEditor-Cmd.exe <FactoryGame.uproject> -run=pythonscript -script="<path>/generate_module_columns.py"

Missing assets are created from their parent class; existing ones are updated in place. Per module: the generated
column components are rebuilt, the class defaults (shared settings, column count, name, description) set; the
descriptor and recipe are pointed at it. Then the schematic is set to unlock all module recipes plus the extra ones,
with a dependency on the required schematics, and registered in the root game world module.
Settings and components the file does not mention are left as they are. Reads the file with PyYAML, which must be
installed in the engine's Python (see README.md, "Editor tools").
"""

from pathlib import Path

import unreal

try:
    import yaml
except ImportError:
    raise SystemExit(
        "PyYAML is not installed in the editor's Python. Close the editor and run:\n"
        '    "<engine>/Engine/Binaries/ThirdParty/Python3/Win64/python.exe" -m pip install pyyaml'
    ) from None

TEMPLATE = Path(__file__).resolve().with_name("module_columns.yaml")
MAX_COLUMNS = 4  # mNumColumns is clamped to 1..4

subsystem = unreal.get_engine_subsystem(unreal.SubobjectDataSubsystem)
data_lib = unreal.SubobjectDataBlueprintFunctionLibrary
asset_lib = unreal.EditorAssetLibrary


def log(message):
    unreal.log(f"[ModuleGenerator] {message}")


def snake_case(name):
    """Python spelling of a C++ property name, e.g. mNumColumns -> m_num_columns."""
    out = ""
    for i, ch in enumerate(name):
        if ch.isupper() and i and (not name[i - 1].isupper() or (i + 1 < len(name) and name[i + 1].islower())):
            out += "_"
        out += ch.lower()
    return out


def set_property(obj, name, value):
    """Sets a property by its C++ name; Python accepts the snake_case spelling, some builds also the original."""
    errors = []
    for candidate in (snake_case(name), name):
        try:
            obj.set_editor_property(candidate, value)
            return
        except Exception as error:  # wrong name or wrong value type
            errors.append(f"{candidate}: {error}")
    raise RuntimeError(f"cannot set {name} on {obj.get_name()}: {'; '.join(errors)}")


def load_class(path):
    cls = unreal.load_class(None, path)
    if cls is None:
        raise RuntimeError(f"class not found: {path}")
    return cls


def load_asset(path):
    asset = unreal.load_asset(path)
    if asset is None:
        raise RuntimeError(f"asset not found: {path}")
    return asset


def to_value(raw):
    """Converts a config value with an optional "class:" / "asset:" / "softclass:" / "text:" prefix."""
    if isinstance(raw, list):
        return [to_value(item) for item in raw]
    if not isinstance(raw, str):
        return raw
    kind, _, path = raw.partition(":")
    if kind == "class":
        return load_class(path)
    if kind == "asset":
        return load_asset(path)
    if kind == "softclass":
        return unreal.SoftClassPath(path)
    if kind == "text":
        return unreal.Text(path)
    return raw


def apply_settings(obj, settings):
    for name, raw in settings.items():
        if name == "parent_class":
            continue
        value = to_value(raw)
        try:
            set_property(obj, name, value)
        except RuntimeError:
            # A soft class reference may be exposed to Python as the class itself.
            if isinstance(raw, str) and raw.startswith("softclass:"):
                set_property(obj, name, load_class(raw.partition(":")[2]))
            elif isinstance(raw, list) and raw and all(isinstance(r, str) and r.startswith("softclass:") for r in raw):
                set_property(obj, name, [load_class(r.partition(":")[2]) for r in raw])
            else:
                raise


def enum_value(enum_name, value):
    """Python enum member for a C++ value name: ESchematicType EST_Milestone -> unreal.SchematicType.MILESTONE."""
    enum = getattr(unreal, enum_name, None) or getattr(unreal, "E" + enum_name, None)
    if enum is None:
        raise RuntimeError(f"enum {enum_name} not exposed to Python")
    short = value.split("_", 1)[1] if "_" in value else value
    for candidate in (snake_case(short).upper(), short.upper(), value.upper(), value):
        member = getattr(enum, candidate, None)
        if member is not None:
            return member
    raise RuntimeError(f"{enum_name} has no value {value}")


def item_amounts(pairs):
    amounts = []
    for path, amount in pairs:
        item = unreal.ItemAmount()
        set_property(item, "ItemClass", load_class(path))
        set_property(item, "Amount", int(amount))
        amounts.append(item)
    return amounts


def defaults_of(blueprint):
    return unreal.get_default_object(blueprint.generated_class())


def load_or_create_blueprint(path, parent_class):
    """Loads a blueprint asset, or creates it with the given parent class."""
    if asset_lib.does_asset_exist(path):
        return load_asset(path)
    package_path, name = path.rsplit("/", 1)
    factory = unreal.BlueprintFactory()
    factory.set_editor_property("parent_class", load_class(parent_class))
    blueprint = unreal.AssetToolsHelpers.get_asset_tools().create_asset(name, package_path, unreal.Blueprint, factory)
    if blueprint is None:
        raise RuntimeError(f"could not create {path}")
    log(f"created {path} (parent {parent_class})")
    return blueprint


def compile_and_save(blueprint, configure):
    """Compiles, lets configure() set the class defaults, and saves."""
    unreal.BlueprintEditorLibrary.compile_blueprint(blueprint)
    configure(defaults_of(blueprint))
    asset_lib.save_loaded_asset(blueprint, False)


# --- Module columns -------------------------------------------------------------------------------------------------

def vector(values):
    return unreal.Vector(*values)


def rotator(values):
    pitch, yaw, roll = values
    return unreal.Rotator(roll=roll, pitch=pitch, yaw=yaw)


def component_class(name):
    cls = getattr(unreal, name, None)
    if cls is None:
        for module in ("Engine", "FactoryGame"):
            cls = unreal.load_class(None, f"/Script/{module}.{name}")
            if cls:
                break
    if cls is None:
        raise RuntimeError(f"unknown component class {name}")
    return cls


def gather(blueprint):
    """Variable name -> subobject handle, plus the actor (context) handle and the root component handle."""
    by_name, context, root = {}, None, None
    for handle in subsystem.k2_gather_subobject_data_for_blueprint(blueprint):
        data = data_lib.get_data(handle)
        if data_lib.is_actor(data):
            context = handle
            continue
        by_name[str(data_lib.get_variable_name(data))] = handle
        if data_lib.is_root_component(data):
            root = handle
    return by_name, context, root


def expand(parts, rows, c, r=None, parent=None):
    """Yields (part, name, row, parent name) for one column, parents before their children.

    A per_row part is expanded once per row; its children are created in that row (row None outside per_row parts).
    """
    for part in parts:
        for row in (range(rows) if part.get("per_row") and r is None else [r]):
            if row is None and "{r}" in part["name"]:
                raise RuntimeError(f"{part['name']}: {{r}} is only allowed in per_row parts and their children")
            name = part["name"].format(c=c, r=row)
            yield part, name, row, parent
            yield from expand(part.get("children", []), rows, c, row, name)


def generated_names(template):
    """Every name the template can generate, for all column indices, in creation order."""
    rows = template["layout"]["rows"]
    return [name for c in range(MAX_COLUMNS) for _, name, _, _ in expand(template["parts"], rows, c)]


def remove_generated(blueprint, template):
    by_name, context, _ = gather(blueprint)
    # Children first, so no generated child is re-parented to the root before it is removed.
    names = list(reversed(generated_names(template))) + list(template["layout"].get("remove_names", []))
    removed = 0
    for name in names:
        handle = by_name.get(name)
        if handle is not None:
            subsystem.delete_subobject(context, handle, blueprint)
            removed += 1
    return removed


def add_part(blueprint, parent, part, name, location, rotation, scale):
    params = unreal.AddNewSubobjectParams(parent_handle=parent, new_class=component_class(part["class"]), blueprint_context=blueprint)
    handle, fail_reason = subsystem.add_new_subobject(params)
    if not data_lib.is_handle_valid(handle):
        raise RuntimeError(f"could not add {name}: {fail_reason}")
    if not subsystem.rename_subobject(handle, unreal.Text(name)):
        raise RuntimeError(f"could not name {name} (name taken?)")
    component = data_lib.get_object(data_lib.get_data(handle))
    component.set_editor_property("relative_location", vector(location))
    component.set_editor_property("relative_rotation", rotator(rotation))
    component.set_editor_property("relative_scale3d", vector(scale))
    if "mesh" in part:
        component.set_editor_property("static_mesh", load_asset(part["mesh"]))
    if "collision_profile" in part:
        component.set_collision_profile_name(part["collision_profile"])
    return handle


def build_columns(blueprint, template, num_columns):
    layout = template["layout"]
    rows = layout["rows"]
    _, _, root = gather(blueprint)
    if root is None:
        raise RuntimeError("no root component")
    for c in range(num_columns):
        offset = [a * (layout["first_column"] + c * layout["spacing"]) for a in layout["axis"]]
        created = {}
        for part, name, row, parent_name in expand(template["parts"], rows, c):
            if part["class"] == "SceneComponent" and "mesh" in part:
                raise RuntimeError(f"{name}: a SceneComponent has no mesh")
            location = part.get("location", [0.0, 0.0, 0.0])
            if part.get("per_row") and "row_locations" in part:
                location = part["row_locations"][row]
            if parent_name is None:
                parent = root
                location = [l + o for l, o in zip(location, offset)]
            else:
                parent = created[parent_name]
            created[name] = add_part(blueprint, parent, part, name, location, part.get("rotation", [0.0, 0.0, 0.0]), part.get("scale", [1.0, 1.0, 1.0]))


# --- Assets ---------------------------------------------------------------------------------------------------------

def generate_module(template, module):
    shared = template["shared"]["module"]
    blueprint = load_or_create_blueprint(module["blueprint"], shared["parent_class"])
    removed = remove_generated(blueprint, template)
    build_columns(blueprint, template, module["columns"])

    def configure(defaults):
        apply_settings(defaults, shared)
        set_property(defaults, "mNumColumns", module["columns"])
        # Buildings take their name from the buildable, not from the descriptor.
        if "display_name" in module:
            set_property(defaults, "mDisplayName", unreal.Text(module["display_name"]))
        if "description" in module:
            set_property(defaults, "mDescription", unreal.Text(module["description"]))

    compile_and_save(blueprint, configure)
    log(f"{module['blueprint']}: removed {removed} components, built {module['columns']} columns")
    return blueprint


def generate_descriptor(template, module, blueprint):
    shared = template["shared"]["descriptor"]
    descriptor = load_or_create_blueprint(module["descriptor"], shared["parent_class"])

    def configure(defaults):
        apply_settings(defaults, shared)
        set_property(defaults, "mBuildableClass", blueprint.generated_class())
        if "menu_priority" in module:
            set_property(defaults, "mMenuPriority", float(module["menu_priority"]))
        if "small_icon" in module:
            set_property(defaults, "mSmallIcon", load_asset(module["small_icon"]))
        if "big_icon" in module:
            set_property(defaults, "mPersistentBigIcon", load_asset(module["big_icon"]))

    compile_and_save(descriptor, configure)
    log(f"{module['descriptor']}: builds {blueprint.get_name()}")
    return descriptor


def generate_recipe(template, module, descriptor):
    shared = template["shared"]["recipe"]
    recipe = load_or_create_blueprint(module["recipe"], shared["parent_class"])

    def configure(defaults):
        apply_settings(defaults, shared)
        set_property(defaults, "mIngredients", item_amounts(module["ingredients"]))
        product = unreal.ItemAmount()
        set_property(product, "ItemClass", descriptor.generated_class())
        set_property(product, "Amount", 1)
        set_property(defaults, "mProduct", [product])

    compile_and_save(recipe, configure)
    log(f"{module['recipe']}: makes {descriptor.get_name()}")
    return recipe


def generate_schematic(template, recipes):
    config = template["schematic"]
    schematic = load_or_create_blueprint(config["path"], config["parent_class"])

    def configure(defaults):
        set_property(defaults, "mType", enum_value("SchematicType", config["type"]))
        set_property(defaults, "mDisplayName", unreal.Text(config["display_name"]))
        set_property(defaults, "mSchematicCategory", load_class(config["category"]))
        set_property(defaults, "mTechTier", int(config["tech_tier"]))
        set_property(defaults, "mMenuPriority", float(config["menu_priority"]))
        set_property(defaults, "mTimeToComplete", float(config["time_to_complete"]))
        set_property(defaults, "mCost", item_amounts(config["cost"]))
        brush = unreal.SlateBrush()
        brush.set_editor_property("resource_object", load_asset(config["icon"]))
        brush.set_editor_property("image_size", unreal.Vector2D(256.0, 256.0))
        set_property(defaults, "mSchematicIcon", brush)

        # Instanced subobjects of the class defaults, rebuilt on every run.
        unlock = unreal.new_object(load_class(config["unlock_class"]), outer=defaults)
        set_property(unlock, "mRecipes", recipes + [load_class(path) for path in config.get("extra_recipes", [])])
        set_property(defaults, "mUnlocks", [unlock])
        dependencies = []
        if config.get("requires"):
            dependency = unreal.new_object(load_class(config["dependency_class"]), outer=defaults)
            set_property(dependency, "mSchematics", [load_class(path) for path in config["requires"]])
            set_property(dependency, "mRequireAllSchematicsToBePurchased", True)
            dependencies.append(dependency)
        set_property(defaults, "mSchematicDependencies", dependencies)

    compile_and_save(schematic, configure)
    log(f"{config['path']}: unlocks {len(recipes) + len(config.get('extra_recipes', []))} recipes")
    return schematic


def register_schematic(template, schematic):
    config = template["schematic"]
    root_module = load_asset(config["register_in"])
    unreal.BlueprintEditorLibrary.compile_blueprint(root_module)
    defaults = defaults_of(root_module)
    current = list(defaults.get_editor_property(snake_case("mSchematics")))
    replaced = {path.rsplit(".", 1)[-1] for path in config.get("replaces", [])}
    kept = [cls for cls in current if cls and cls.get_name() not in replaced and cls != schematic.generated_class()]
    set_property(defaults, "mSchematics", kept + [schematic.generated_class()])
    asset_lib.save_loaded_asset(root_module, False)
    log(f"{config['register_in']}: schematics {[cls.get_name() for cls in kept + [schematic.generated_class()]]}")


def main():
    template = yaml.safe_load(TEMPLATE.read_text(encoding="utf-8"))
    recipes = []
    for module in template["modules"]:
        blueprint = generate_module(template, module)
        descriptor = generate_descriptor(template, module, blueprint)
        recipe = generate_recipe(template, module, descriptor)
        recipes.append(recipe.generated_class())
    schematic = generate_schematic(template, recipes)
    register_schematic(template, schematic)
    log("done")


main()
