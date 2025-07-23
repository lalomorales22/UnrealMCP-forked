"""Unreal Engine action implementations for the AI Assistant."""

from typing import Any, Dict

import unreal


def _get_selected_actors() -> list[unreal.Actor]:
    """Return all currently selected actors in the editor."""
    return list(unreal.EditorLevelLibrary.get_selected_level_actors())


def create_object(entities: Dict[str, Any], context: Dict[str, Any]) -> str:
    """Spawn a basic actor like a cube or sphere or from an asset path."""
    object_type = entities.get("object_type")
    position = entities.get("position", {"x": 0, "y": 0, "z": 0})
    asset_path = entities.get("asset_path")

    mesh_path = asset_path
    if object_type in {"cube", "box"}:
        mesh_path = mesh_path or "/Engine/BasicShapes/Cube.Cube"
    elif object_type == "sphere":
        mesh_path = mesh_path or "/Engine/BasicShapes/Sphere.Sphere"

    if mesh_path is None:
        return f"Unsupported object type: {object_type}"

    location = unreal.Vector(position.get("x", 0), position.get("y", 0), position.get("z", 0))
    rotation = unreal.Rotator(0.0, 0.0, 0.0)
    mesh = unreal.EditorAssetLibrary.load_asset(mesh_path)
    actor = unreal.EditorLevelLibrary.spawn_actor_from_object(mesh, location, rotation)
    return f"Created {actor.get_name()}"


def move_object(entities: Dict[str, Any], context: Dict[str, Any]) -> str:
    """Move selected actors to a new location."""
    position = entities.get("position")
    if not position:
        return "No position specified"

    location = unreal.Vector(position.get("x", 0), position.get("y", 0), position.get("z", 0))
    actors = context.get("actors") or _get_selected_actors()
    for actor in actors:
        actor.set_actor_location(location, False, True)
    return f"Moved {len(actors)} actor(s)"


def delete_object(entities: Dict[str, Any], context: Dict[str, Any]) -> str:
    """Delete selected actors from the level."""
    actors = context.get("actors") or _get_selected_actors()
    for actor in actors:
        unreal.EditorLevelLibrary.destroy_actor(actor)
    return f"Deleted {len(actors)} actor(s)"


def rotate_object(entities: Dict[str, Any], context: Dict[str, Any]) -> str:
    """Rotate selected actors."""
    rotation = entities.get("rotation", {})
    rotator = unreal.Rotator(rotation.get("pitch", 0.0), rotation.get("yaw", 0.0), rotation.get("roll", 0.0))
    actors = context.get("actors") or _get_selected_actors()
    for actor in actors:
        actor.set_actor_rotation(rotator, False)
    return f"Rotated {len(actors)} actor(s)"


def scale_object(entities: Dict[str, Any], context: Dict[str, Any]) -> str:
    """Scale selected actors."""
    scale = entities.get("scale", {})
    scale_vector = unreal.Vector(scale.get("x", 1.0), scale.get("y", 1.0), scale.get("z", 1.0))
    actors = context.get("actors") or _get_selected_actors()
    for actor in actors:
        actor.set_actor_scale3d(scale_vector)
    return f"Scaled {len(actors)} actor(s)"
