=============================================
DoN AI Navigation Plugin for Unreal Engine 4
=============================================

This plugin provides a 3D dynamic pathfinding system for use with Unreal Engine 4. It's primarly designed for Flying AI creatures based in dynamic or procedural worlds that need to solve complex pathfinding tasks and other behavior that cannot be readily solved  by Unreal's native AI navigation system and also for maps that are too complex to be solved by simple ray-tracing/sweeping heuristics or by waypoint systems. 

Where possible, it recommended that you use Unreal's native AI navigation or simpler navigation strategies; use this system when more conventional strategies cannot solve your pathfinding usecases.

The plugin provides the following:
* A Navigation Manager actor for configuring the system
* "Fly To" behavior tree node that can be readily dropped into your behavior trees
* Nodes for managing dynamic collision and pathfinding in your scene.
* A pathfinding API that advanced users can use for custom navigation queries from either Blueprints or C++

For a technical overview of the project, please visit this link:<br>
http://www.drunkonnectar.com/3d-pathfinding-ue4/#TechnicalOverview
