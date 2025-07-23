from typing import Any, Callable, Dict, Optional

import threading
import unreal

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
            "set_material": unreal_actions.set_material,
            "bake_lighting": unreal_actions.bake_lighting,
        }
        # Intents that may take a long time and should be executed asynchronously
        self.async_intents = {"set_material", "bake_lighting"}

    def dispatch_command(self, command: Dict[str, Any]) -> str:
        """Execute an action based on the parsed command synchronously."""
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

    def dispatch_command_async(
        self,
        command: Dict[str, Any],
        callback: Optional[Callable[[str], None]] = None,
    ) -> str:
        """Execute an action, running long tasks on a separate thread."""
        intent = command.get("intent")
        entities = command.get("entities", {})
        context = command.get("context", {})

        if intent is None:
            return "No intent detected."

        action = self.intent_map.get(intent)
        if not action:
            return f"Unknown intent: {intent}"

        def _task() -> None:
            try:
                result = action(entities, context)
            except Exception as exc:  # pragma: no cover - relies on Unreal API
                result = f"Error executing {intent}: {exc}"

            if callback:
                try:
                    callback(result)
                except Exception as cb_exc:  # pragma: no cover - best effort
                    unreal.log_warning(f"Callback error: {cb_exc}")
            else:
                unreal.log(result)

        if intent in self.async_intents:
            thread = threading.Thread(target=_task, daemon=True)
            thread.start()
            return "Working on it..."

        # Fall back to synchronous execution
        return self.dispatch_command(command)
