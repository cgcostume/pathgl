pathgl
======

Path traceing test for triangle meshes, utilizing the GPU. In each frame, one path per pixel is traced and accumulated with all previous rays of that pixel, thus continuously reducing noise over time.

Check out the dirty source code (its probably plattform independent - not tested yet) or try out the windows x64 demo: http://pathgl.googlecode.com/files/pathgl_v1_a2a36eb7c12c.7z

Missing in Action (todo):

* Antialiasing
* Materials/BRDF
* Correct Path Color Accumulation
* Reflection
* Bounding Volume Hierarchies
* ... more

