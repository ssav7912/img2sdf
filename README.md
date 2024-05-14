# IMG2SDF

![df.gif](df.gif)

A small utility for generating (un)signed distance fields
and voronoi diagrams using the Jump Flood algorithm. 
Implemented in DX11 compute shaders.

## Building
This project uses CMake. It depends on the Windows Runtime Library,
and the DX11 toolkit. Other external dependencies include argparse
and googletest, which are automatically downloaded by CMake.

A number of targets are provided:
`libimg2sdf`: The main library. Compiled as a static lib,
the primary interface is through the `img2sdf.h` header.

`img2sdf`: this is a utility command line tool that will
generate normalised unsigned distance field or voronoi diagram
from the input image path provided. 

`profile`: A small profiling program that reports timings
for the various algorithms used in this project.

`test`: Test cases used in this project.

## Usage

Link against `libimg2sdf`. Include the `img2sdf.h` header.
Initialise the `Img2SDF` object with your DX11 device and context. Note that `Windows::Foundation::Initialize` must have been called
at program startup. Call the method
of your choice with an input `ID3D11Texture2D` to get back a computed
`ID3D11Texture2D`:
```cpp
    ///initalise your device, context and texture somewhere up here...
    Img2SDF img2sdf {device.Get(), context.Get()};
    ComPtr<ID3D11Texture2D> out_texture = img2sdf.compute_signed_distance_field(in_texture);
```
The input texture must have a size of a power of 2, and must be an R32 float texture. Any pixel
that is non-zero is treated as an input seed.

## Results

Below are the coarse timings for the jumpflooding functions provided. These were created
by generating seed textures with random distributions of seed pixels of varying sizes, and
computing each of the functions for them. Timing information is retrieved using the technique described here: https://therealmjp.github.io/posts/profiling-in-dx11-with-queries/,
with 1000 runs per benchmark per resource size. All timings are in milliseconds.


|Function Name|8x8       |16x16     |32x32     |64x64     |128x128   |256x256   |512x512   |1024x1024 |2048x2048  |4096x4096  |8192x192   |
|-------------|----------|----------|----------|----------|----------|----------|----------|----------|-----------|-----------|-----------|
|voronoi diagram|0.02756   |0.02956   |0.03236   |0.03552   |0.06192   |0.15884   |0.6354    |3.18748   |6.74952    |28.49016   |123.90044  |
|unsigned distance field|0.40905   |0.15913   |0.15968   |0.18228   |0.45768   |0.34281   |0.81117   |3.21409   |8.89338    |31.78636   |131.29762  |
|signed distance field|0.2551    |0.21757   |0.22282   |0.22588   |0.28389   |0.54302   |1.39451   |5.98958   |11.48001   |54.56952   |256.89974  |

Graphed below (log scale):
![img_2.png](img_2.png)

This demonstrates a loglinear performance, which matches the big-O bounds for the algorithm. 
For small textures, the runtime is dominated by the minmax reduction that needs to be done to normalise the output,
particularly as my implementation falls back to a serial (CPU) minimax algorithm when the reduction produces a UAV smaller than
a wavefront (8x8 pixels). This means that the benefits of a parallel reduction don't really start to appear until our textures
are 128x128 pixels in size. 

![img_1.png](img_1.png)

Removing the normalisation pass eliminates most of this overhead for small textures. Computing the signed
distance field takes double the time as the current implementation performs two separate unsigned distance passes, one on an inverted source,
and composites the result in later. This can most likely be merged into a single pass.

|Function Name|8x8       |16x16     |32x32     |64x64     |128x128   |256x256   |512x512   |1024x1024 |2048x2048  |4096x4096  |8192x192   |
|-------------|----------|----------|----------|----------|----------|----------|----------|----------|-----------|-----------|-----------|
|voronoi diagram (unnormalised)|0.0242196 |0.02931456|0.03124156|0.03569368|0.05721184|0.15094444|0.5695222 |2.50745572|9.3002798  |30.25394848|124.9395971|
|unsigned distance field (unnormalised)|0.02699344|0.0312328 |0.03323916|0.0380942 |0.05935462|0.15259252|0.58686816|2.56998052|9.37712678 |29.6991648 |128.9554467|
|signed distance field (unnormalised)|0.05367596|0.05955512|0.06637152|0.07835928|0.12483532|0.316812  |1.183537  |5.0593942 |11.3952964 |55.89069016|255.8165938|

Finer grain profiling was also done in each individual kernel in the pipeline. The profiling includes the time spent on the
CPU to create any required resources (aside from dependencies on other kernels), and includes timings for multiple dispatches
where that's needed for a given shader, such as the minmax reduction and the voronoi shader.

Below are the
results for 1000 runs on a 2048x2048 texture:
![img_3.png](img_3.png)

|Kernel    |min (ms)  |max (ms)  |average time (ms)|
|----------|----------|----------|-----------------|
|Preprocess Shader|0.24804   |0.81044   |0.44811532       |
|Voronoi Shader|4.62988   |11.18076  |9.35256104       |
|Distance Shader|0.10284   |0.4724    |0.17538288       |
|MinMax Reduce Shader|0.0958    |0.77176   |0.27933844       |
|Distance Normalise Shader|0.04496   |0.18292   |0.1082066        |
|Composite Shader|0.0268    |0.13744   |0.08472964       |

Surprisingly, the vast majority of time is spent in the jump-flood shader itself (Voronoi Shader). I was expecting the 
min-max reduction to have a higher overhead due to its poor thread residency. Warrants further investigation.   

## References
https://therealmjp.github.io/posts/profiling-in-dx11-with-queries/
https://blog.demofox.org/2016/02/29/fast-voronoi-diagrams-and-distance-dield-textures-on-the-gpu-with-the-jump-flooding-algorithm/
https://www.comp.nus.edu.sg/~tants/jfa/i3d06.pdf 