### Mesh Intersection Volume Computation

Using a [particular formula by W. Randolph Franklin][wrf], we can find the
intersection volume of two triangle meshes without having to compute the
intersection mesh itself. We can use a CUDA-enabled GPU to speed up the
computation.

This implementation is provided as part of my bachelor thesis, available
in the `thesis` directory.
A comprehensive documentation of the code is still pending.


[wrf]: https://wrf.ecse.rpi.edu//Research/Short_Notes/volume.html
