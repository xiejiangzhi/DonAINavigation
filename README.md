# DoN AI Navigation Plugin for Unreal Engine 4
This plugin provides a 3D dynamic pathfinding system for use with Unreal Engine 4. This system was first developed for [DoN The Nature Game](http://www.drunkonnectar.com/) and the owner decided to make a plugin out of the navigation module as a gift to the Unreal community. He abandoned the project some years back and I started to collect various fixes and pull requests after I updated it to the current version.

The plugin is primarly designed for Flying AIs based in dynamic or procedural worlds, which need to solve complex pathfinding tasks and other behavior not covered by Unreal's native AI navigation system or maps, which are too complex to be solved by simple ray-tracing/sweeping heuristics or a waypoint system.

Where possible, I recommend that you use Unreal's native AI navigation or simpler navigation strategies; use this system when the more conventional strategies cannot solve your pathfinding usecases.

The plugin provides the following:
* Navigation Manager actor for configuring the system
* "Fly To" behavior tree node that can be readily dropped into your behavior trees
* Nodes for managing dynamic collision and pathfinding in your scene.
* A pathfinding API that advanced users can use for custom navigation queries from either Blueprints or C++

# Sample Project
There is still is an old [sample project](http://www.drunkonnectar.com/3d-pathfinding-ue4/) to quickly test the system and understand the different usecases it covers.

# Overview and Tutorial
[![Youtube video - Overview and Tutorial](http://www.drunkonnectar.com/wp-content/uploads/2016/03/ThumbnailWithYoutubeIcon.jpg)](https://www.youtube.com/watch?v=6Tr_K551zvI)

# Technical Overview
For a technical overview of the project, please visit this [link](http://www.drunkonnectar.com/3d-pathfinding-ue4/#TechnicalOverview)

