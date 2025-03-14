def register_tools(mcp, utils):    
    send_command = utils['send_command']

    @mcp.tool()
    def my_custom_tool(ctx):
        return "Hello from custom tool!"
    
    @mcp.tool()
    def get_actor_count(ctx) -> str:
        """Get the number of actors in the current Unreal Engine scene."""
        try:
            response = send_command("get_scene_info")
            print(f"Response: {response}")
            if response["status"] == "success":
                result = response["result"]
                total_actor_count = result["actor_count"]
                returned_actor_count = result.get("returned_actor_count", len(result["actors"]))
                limit_reached = result.get("limit_reached", False)
                
                response_text = f"Total number of actors: {total_actor_count}\n"
                
                if limit_reached:
                    response_text += f"WARNING: Actor limit reached! Only {returned_actor_count} actors were returned in the response.\n"
                    response_text += f"The remaining {total_actor_count - returned_actor_count} actors are not included in the response.\n"
                
                return response_text
            else:
                return f"Error: {response['message']}"
        except Exception as e:
            return f"Error: {str(e)}"
        
