﻿#pragma once
#ifndef __SHADER_CREATOR_HPP__
#define __SHADER_CREATOR_HPP__

#include "Shader.hpp"
#include "Lambertian.hpp"
#include "Glass.hpp"
namespace MyRender
{
    class ShaderCreator
    {
    public:
        ShaderCreator() = default;
        SharedShader create(Material& material, vector<Texture>& t) {
            SharedShader shader{nullptr};
            switch (material.type)
            {
             case 0:
            case 1:
                shader = make_shared<Lambertian>(material, t);
                break;
            case 2:
            case 3:
                shader = make_shared<Glass>(material, t);
                break;
            default:
                shader = make_shared<Lambertian>(material, t);
                break;
            }
            return shader;
        }
    };
}

#endif