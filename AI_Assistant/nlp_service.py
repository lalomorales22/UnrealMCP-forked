"""Simple keyword based NLP service with editor context awareness."""
import re
from typing import Any, Dict

import unreal


class NLPService:
    def __init__(self) -> None:
        self.intent_keywords = {
            "create": ["create", "make", "add", "spawn"],
            "delete": ["delete", "remove", "destroy"],
            "move": ["move", "translate", "position"],
            "rotate": ["rotate", "turn", "orient"],
            "scale": ["scale", "resize"],
            "select": ["select", "find", "get"],
            "set_material": ["material", "texture", "apply material"],
            "get_info": ["info", "details", "properties", "what is"],
        }

    def parse_command(self, text: str) -> Dict[str, Any]:
        text_lower = text.lower()

        # capture currently selected actors in the editor
        try:
            selected = list(unreal.EditorLevelLibrary.get_selected_level_actors())
        except Exception:
            selected = []
        context = {"actors": selected}

        intent = None
        for key, kws in self.intent_keywords.items():
            if any(kw in text_lower for kw in kws):
                intent = key
                break

        entities: Dict[str, Any] = {}

        # enhanced regex for position extraction: x 100.5 y -50 z 0
        position_match = re.search(
            r"x\s*(-?\d+(?:\.\d+)?)\s*y\s*(-?\d+(?:\.\d+)?)\s*z\s*(-?\d+(?:\.\d+)?)",
            text_lower,
        )
        if position_match:
            entities["position"] = {
                "x": float(position_match.group(1)),
                "y": float(position_match.group(2)),
                "z": float(position_match.group(3)),
            }

        # rotation: pitch 30 yaw 60 roll 0 (any order, values optional)
        rot = {}
        pitch_match = re.search(r"pitch\s*(-?\d+(?:\.\d+)?)", text_lower)
        yaw_match = re.search(r"yaw\s*(-?\d+(?:\.\d+)?)", text_lower)
        roll_match = re.search(r"roll\s*(-?\d+(?:\.\d+)?)", text_lower)
        if pitch_match:
            rot["pitch"] = float(pitch_match.group(1))
        if yaw_match:
            rot["yaw"] = float(yaw_match.group(1))
        if roll_match:
            rot["roll"] = float(roll_match.group(1))
        if rot:
            entities["rotation"] = rot

        # scale: either uniform 'scale 2' or axes 'scale x 1 y 2 z 3'
        scale_xyz_match = re.search(
            r"scale\s*x\s*(-?\d+(?:\.\d+)?)\s*y\s*(-?\d+(?:\.\d+)?)\s*z\s*(-?\d+(?:\.\d+)?)",
            text_lower,
        )
        if scale_xyz_match:
            entities["scale"] = {
                "x": float(scale_xyz_match.group(1)),
                "y": float(scale_xyz_match.group(2)),
                "z": float(scale_xyz_match.group(3)),
            }
        else:
            scale_uniform = re.search(r"scale\s*(-?\d+(?:\.\d+)?)", text_lower)
            if scale_uniform:
                val = float(scale_uniform.group(1))
                entities["scale"] = {"x": val, "y": val, "z": val}

        # capture asset paths like '/Game/Path/Asset.Asset'
        asset_match = re.search(r"(/[-\w/]+(?:\.[-\w]+)?)", text)
        if asset_match:
            entities["asset_path"] = asset_match.group(1)

        # extract simple object type (first word after intent)
        if intent:
            pattern = rf"{intent}\s+(\w+)"
            obj_match = re.search(pattern, text_lower)
            if obj_match:
                entities["object_type"] = obj_match.group(1)

        return {"intent": intent, "entities": entities, "context": context}
