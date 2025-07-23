"""Simple keyword based NLP service."""
import re
from typing import Dict, Any


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
        intent = None
        for key, kws in self.intent_keywords.items():
            if any(kw in text_lower for kw in kws):
                intent = key
                break

        entities: Dict[str, Any] = {}

        # crude regex for position extraction: x 100 y 50 z 0
        position_match = re.search(r"x\s*(-?\d+)\s*y\s*(-?\d+)\s*z\s*(-?\d+)", text_lower)
        if position_match:
            entities["position"] = {
                "x": int(position_match.group(1)),
                "y": int(position_match.group(2)),
                "z": int(position_match.group(3)),
            }

        # extract simple object type (first word after intent)
        if intent:
            pattern = rf"{intent}\s+(\w+)"
            obj_match = re.search(pattern, text_lower)
            if obj_match:
                entities["object_type"] = obj_match.group(1)

        return {"intent": intent, "entities": entities}
