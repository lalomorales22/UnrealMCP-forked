from typing import Any, Callable, Dict

from . import unreal_actions


class CommandDispatcher:
    """Map parsed intents to Unreal action functions."""

    def __init__(self) -> None:
        self.intent_map: Dict[str, Callable[[Dict[str, Any], Dict[str, Any]], str]] = {
            "create": unreal_actions.create_object,
            "move": unreal_actions.move_object,
            "delete": unreal_actions.delete_object,
            "rotate": unreal_actions.rotate_object,
            "scale": unreal_actions.scale_object,
        }

    def dispatch_command(self, command: Dict[str, Any]) -> str:
        """Execute an action based on the parsed command."""
        intent = command.get("intent")
        entities = command.get("entities", {})
        context = command.get("context", {})

        if intent is None:
            return "No intent detected."

        action = self.intent_map.get(intent)
        if not action:
            return f"Unknown intent: {intent}"

        try:
            return action(entities, context)
        except Exception as exc:  # pragma: no cover - relies on Unreal API
            return f"Error executing {intent}: {exc}"
