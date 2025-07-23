"""Utility to spawn the AI Assistant widget inside the editor."""
import unreal

from .nlp_service import NLPService
from .command_dispatcher import CommandDispatcher

ASSET_PATH = "/Game/AI_Assistant/WBP_AI_Assistant"


# Persistent instances to keep conversational memory
_SERVICE = NLPService()
_DISPATCHER = CommandDispatcher()


def _on_async_complete(message: str) -> None:
    """Log asynchronous task completion in the editor."""
    unreal.log(message)


def open_ai_assistant():
    """Open the AI assistant widget if it exists."""
    subsystem = unreal.get_editor_subsystem(unreal.EditorUtilitySubsystem)
    if unreal.EditorAssetLibrary.does_asset_exist(ASSET_PATH):
        subsystem.spawn_and_register_tab(ASSET_PATH)
    else:
        unreal.log_warning(f"AI Assistant widget not found: {ASSET_PATH}")


def process_user_command(text: str):
    """Entry point called from the UI Blueprint."""
    command = _SERVICE.parse_command(text)
    result = _DISPATCHER.dispatch_command_async(command, _on_async_complete)
    return result
