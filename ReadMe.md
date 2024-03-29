# jit.mo
![CI](https://github.com/Cycling74/jit.mo/actions/workflows/test.yml/badge.svg)

Generate realtime motion graphics in Jitter

This package contains a series of new externals to ease creation of generative animations and geometry multiples in Jitter. The jit.mo objects all tie into a global animation graph and are implicitly connected to jit.world. This allows for smooth and continuous realtime animations that factor in framerate imperfections. The jit.mo package is optimized for use with **jit.gl.multiple** or **jit.gl.mesh** to create complex generative animations with a minimum of patching.

- **jit.mo.func** generates a 1d Jitter Matrix using a built-in function (line, sin, saw, tri, perlin) that can be animated over time in sync with the rendering context
- **jit.mo.join** combines and manages the connected jit.mo.func object outputs into a multiplane matrix. Unlike most Jitter objects, the inputs to jit.mo.join are additive and can support multiple connections, making jit.mo additive synthesis possible.
- **jit.mo.field** takes a matrix of position values and manipulates them based on their relationship to a center point
- **jit.mo.time** generates float values according to different time modes (accum, function, delta)


## Contributors / Acknowledgements

* See the [GitHub Contributor Graph](https://github.com/Cycling74/jit.mo/graphs/contributors)

## Support

For support, please use the developer forums at:
http://cycling74.com/forums/
