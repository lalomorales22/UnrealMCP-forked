"""Utility to spawn the AI Assistant widget inside the editor."""
import unreal

ASSET_PATH = "/Game/AI_Assistant/WBP_AI_Assistant"


def open_ai_assistant():
    """Open the AI assistant widget if it exists."""
    subsystem = unreal.get_editor_subsystem(unreal.EditorUtilitySubsystem)
    if unreal.EditorAssetLibrary.does_asset_exist(ASSET_PATH):
        subsystem.spawn_and_register_tab(ASSET_PATH)
    else:
        unreal.log_warning(f"AI Assistant widget not found: {ASSET_PATH}")


def process_user_command(text: str):
    """Entry point called from the UI Blueprint."""
    from .nlp_service import NLPService
    from .command_dispatcher import CommandDispatcher

    service = NLPService()
    dispatcher = CommandDispatcher()

    command = service.parse_command(text)
    result = dispatcher.dispatch_command(command)

    return result
