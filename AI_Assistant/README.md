# AI Assistant for Unreal Engine

This folder contains Python utilities for the in-editor AI Assistant.

## Phase 1: Chat UI Setup

1. **Create an Editor Utility Widget** named `WBP_AI_Assistant` inside your project.
2. Design the layout with the following elements:
   - `ChatHistoryScrollBox` (ScrollBox)
   - `ChatHistoryVerticalBox` (VerticalBox inside the scroll box)
   - `UserInputTextBox` (MultiLineEditableTextBox)
   - `SendButton` (Button)
3. Create a second widget `WBP_ChatMessage` containing a `TextBlock` and an exposed parameter for the sender.
4. Bind the `SendButton` click event to read the text, create a `WBP_ChatMessage`, add it to the vertical box, clear the input field and call a Python function named `process_user_command(text)`.
5. Add a toolbar button or menu entry that opens `WBP_AI_Assistant` so it can be launched from the editor.

The Python modules in this directory provide the backend logic to process user commands and can be called from the widget's graph.

## Phase 2: Context-Aware Commands

`nlp_service.py` now collects the currently selected actors in the editor and passes
this context to the action functions. Commands like "move it up by 200" operate
on the selection without needing an explicit object name. The NLP parser also
extracts more transformation values, including rotation and scaling.
