OpenGL-Cloth
============

A simple mesh cloth made with OpenGL

Thanks to this tutorial: 
http://gamedev.tutsplus.com/tutorials/implementation/simulate-fabric-and-ragdolls-with-simple-verlet-integration/

Essentially, the links in the mesh are modelled as simple springs. When they get stretched too far, they break.

Left click to grab points in the mesh, pulling too hard will break links.
Left and right arrows can spin the camera.
Space randomly flutters the mesh.
ESC quits.

Simple makefile included, so all you need to do is type "make".
Requires OpenGL, GLU, and glfw (which probably requires a few things for it to work). 

Feel free to use this code (although right now, it's kind of poorly organized...all in one big file).
For any legal purposes, consider this Public Domain.

Sean Hickey
(Wisellama)
