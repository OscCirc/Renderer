#pragma once

#include <array>

class Framebuffer;

template <typename Attribs, typename Varyings, typename Uniforms>
class Program;


template <typename Attribs, typename Varyings, typename Uniforms>
void rasterize_polygon(Framebuffer *framebuffer, Program<Attribs, Varyings, Uniforms> *program, int num_vertices);