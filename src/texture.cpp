#include "texture.h"
#include "CGL/color.h"

namespace CGL {

Color Texture::sample(const SampleParams &sp) {
  // // Part 5: Fill this in.
  // if (sp.psm == P_NEAREST) {
  //   return sample_nearest(sp.p_uv, 0);
  // } else {
  //   return sample_bilinear(sp.p_uv, 0);
  // }

  // Part 6
  int level;
  if (sp.lsm == L_NEAREST) {
    level = std::max(0, std::min((int) floor(get_level(sp) + 0.5), (int) (mipmap.size() - 1)));
  } else if (sp.lsm == L_LINEAR) { // L_LINEAR
    // Note, this implementation uses sp.psm to perform different trilinear sampling
    return sample_trilinear(sp);
  } else { // L_ZERO
    level = 0;
  }

  if (sp.psm == P_NEAREST) {
    return sample_nearest(sp.p_uv, level);
  } else { // P_LINEAR
    return sample_bilinear(sp.p_uv, level);
  } 
}

float Texture::get_level(const SampleParams &sp) {
  // Part 6: Fill this in.
  Vector2D dx = (sp.p_dx_uv - sp.p_uv) * width;
  Vector2D dy = (sp.p_dy_uv - sp.p_uv) * height;

  float v1 = (float) sqrt(dot(dx, dx));
  float v2 = (float) sqrt(dot(dy, dy));
  float L = std::max(v1, v2);

  return log2f(L);
}

Color Texture::sample_nearest(Vector2D uv, int level) {
  // Part 5: Fill this in.
  int width = mipmap[level].width;
  int height = mipmap[level].height;

  int x = std::max(0, std::min((int) floor(uv.x * width + 0.5), width-1));
  int y = std::max(0, std::min((int) floor(uv.y * height + 0.5), height-1));
  int idx = 4 * (y * width + x);

  return Color(&mipmap[level].texels[idx]);
}

Color Texture::sample_bilinear(Vector2D uv, int level) {
  // Part 5: Fill this in.
  struct miplevel_helper {
    Color get(MipLevel &mip_level, int row, int col) {
      float r, g, b, a;
      int idx = 4 * (row * mip_level.width + col);
      return Color(&mip_level.texels[idx]);
    } 
  } helper;

  int width = mipmap[level].width;
  int height = mipmap[level].height;
  int x = floor(uv.x * width), y = floor(uv.y * height);

  Color u00 =  helper.get(mipmap[level], y, x);
  Color u01 =  helper.get(mipmap[level], y, std::min((int) (width - 1), x+1));
  Color u10 =  helper.get(mipmap[level], std::min((int) (height - 1), y+1), x);
  Color u11 =  helper.get(mipmap[level], std::min((int) (height - 1), y+1), 
                                        std::min((int) (width - 1), x+1));

  float s = uv.x * width - x, t = uv.y * height - y;
  Color u0 = (1 - s) * u00 + s * u10;
  Color u1 = (1 - s) * u01 + s * u11;

  return (1 - t) * u0 + t * u1;
}

//Color Texture::sample_trilinear(Vector2D uv, Vector2D du, Vector2D dv) {
Color Texture::sample_trilinear(const SampleParams &sp) {
  // Part 6: Fill this in.
  float level = get_level(sp);
  float w1 = level - floor(level), w2 = ceil(level) - level;

  int l1 = std::max(0, (int) floor(level));
  int l2 = std::max(0, std::min((int) (mipmap.size() - 1), (int) floor(level + 1)));
  
  if (sp.psm == P_NEAREST)
  {
    Color c1 = sample_nearest(sp.p_uv, l1);
    Color c2 = sample_nearest(sp.p_uv, l2);
    return w1 * c1 + w2 * c2;
  } else {
    Color c1 = sample_bilinear(sp.p_uv, l1);
    Color c2 = sample_bilinear(sp.p_uv, l2);
    return w1 * c1 + w2 * c2;
  }
}


/****************************************************************************/



inline void uint8_to_float(float dst[4], unsigned char *src) {
  uint8_t *src_uint8 = (uint8_t *)src;
  dst[0] = src_uint8[0] / 255.f;
  dst[1] = src_uint8[1] / 255.f;
  dst[2] = src_uint8[2] / 255.f;
  dst[3] = src_uint8[3] / 255.f;
}

inline void float_to_uint8(unsigned char *dst, float src[4]) {
  uint8_t *dst_uint8 = (uint8_t *)dst;
  dst_uint8[0] = (uint8_t)(255.f * max(0.0f, min(1.0f, src[0])));
  dst_uint8[1] = (uint8_t)(255.f * max(0.0f, min(1.0f, src[1])));
  dst_uint8[2] = (uint8_t)(255.f * max(0.0f, min(1.0f, src[2])));
  dst_uint8[3] = (uint8_t)(255.f * max(0.0f, min(1.0f, src[3])));
}

void Texture::generate_mips(int startLevel) {

  // make sure there's a valid texture
  if (startLevel >= mipmap.size()) {
    std::cerr << "Invalid start level";
  }

  // allocate sublevels
  int baseWidth = mipmap[startLevel].width;
  int baseHeight = mipmap[startLevel].height;
  int numSubLevels = (int)(log2f((float)max(baseWidth, baseHeight)));

  numSubLevels = min(numSubLevels, kMaxMipLevels - startLevel - 1);
  mipmap.resize(startLevel + numSubLevels + 1);

  int width = baseWidth;
  int height = baseHeight;
  for (int i = 1; i <= numSubLevels; i++) {

    MipLevel &level = mipmap[startLevel + i];

    // handle odd size texture by rounding down
    width = max(1, width / 2);
    //assert (width > 0);
    height = max(1, height / 2);
    //assert (height > 0);

    level.width = width;
    level.height = height;
    level.texels = vector<unsigned char>(4 * width * height);
  }

  // create mips
  int subLevels = numSubLevels - (startLevel + 1);
  for (int mipLevel = startLevel + 1; mipLevel < startLevel + subLevels + 1;
       mipLevel++) {

    MipLevel &prevLevel = mipmap[mipLevel - 1];
    MipLevel &currLevel = mipmap[mipLevel];

    int prevLevelPitch = prevLevel.width * 4; // 32 bit RGBA
    int currLevelPitch = currLevel.width * 4; // 32 bit RGBA

    unsigned char *prevLevelMem;
    unsigned char *currLevelMem;

    currLevelMem = (unsigned char *)&currLevel.texels[0];
    prevLevelMem = (unsigned char *)&prevLevel.texels[0];

    float wDecimal, wNorm, wWeight[3];
    int wSupport;
    float hDecimal, hNorm, hWeight[3];
    int hSupport;

    float result[4];
    float input[4];

    // conditional differentiates no rounding case from round down case
    if (prevLevel.width & 1) {
      wSupport = 3;
      wDecimal = 1.0f / (float)currLevel.width;
    } else {
      wSupport = 2;
      wDecimal = 0.0f;
    }

    // conditional differentiates no rounding case from round down case
    if (prevLevel.height & 1) {
      hSupport = 3;
      hDecimal = 1.0f / (float)currLevel.height;
    } else {
      hSupport = 2;
      hDecimal = 0.0f;
    }

    wNorm = 1.0f / (2.0f + wDecimal);
    hNorm = 1.0f / (2.0f + hDecimal);

    // case 1: reduction only in horizontal size (vertical size is 1)
    if (currLevel.height == prevLevel.height) {
      //assert (currLevel.height == 1);

      for (int i = 0; i < currLevel.width; i++) {
        wWeight[0] = wNorm * (1.0f - wDecimal * i);
        wWeight[1] = wNorm * 1.0f;
        wWeight[2] = wNorm * wDecimal * (i + 1);

        result[0] = result[1] = result[2] = result[3] = 0.0f;

        for (int ii = 0; ii < wSupport; ii++) {
          uint8_to_float(input, prevLevelMem + 4 * (2 * i + ii));
          result[0] += wWeight[ii] * input[0];
          result[1] += wWeight[ii] * input[1];
          result[2] += wWeight[ii] * input[2];
          result[3] += wWeight[ii] * input[3];
        }

        // convert back to format of the texture
        float_to_uint8(currLevelMem + (4 * i), result);
      }

      // case 2: reduction only in vertical size (horizontal size is 1)
    } else if (currLevel.width == prevLevel.width) {
      //assert (currLevel.width == 1);

      for (int j = 0; j < currLevel.height; j++) {
        hWeight[0] = hNorm * (1.0f - hDecimal * j);
        hWeight[1] = hNorm;
        hWeight[2] = hNorm * hDecimal * (j + 1);

        result[0] = result[1] = result[2] = result[3] = 0.0f;
        for (int jj = 0; jj < hSupport; jj++) {
          uint8_to_float(input, prevLevelMem + prevLevelPitch * (2 * j + jj));
          result[0] += hWeight[jj] * input[0];
          result[1] += hWeight[jj] * input[1];
          result[2] += hWeight[jj] * input[2];
          result[3] += hWeight[jj] * input[3];
        }

        // convert back to format of the texture
        float_to_uint8(currLevelMem + (currLevelPitch * j), result);
      }

      // case 3: reduction in both horizontal and vertical size
    } else {

      for (int j = 0; j < currLevel.height; j++) {
        hWeight[0] = hNorm * (1.0f - hDecimal * j);
        hWeight[1] = hNorm;
        hWeight[2] = hNorm * hDecimal * (j + 1);

        for (int i = 0; i < currLevel.width; i++) {
          wWeight[0] = wNorm * (1.0f - wDecimal * i);
          wWeight[1] = wNorm * 1.0f;
          wWeight[2] = wNorm * wDecimal * (i + 1);

          result[0] = result[1] = result[2] = result[3] = 0.0f;

          // convolve source image with a trapezoidal filter.
          // in the case of no rounding this is just a box filter of width 2.
          // in the general case, the support region is 3x3.
          for (int jj = 0; jj < hSupport; jj++)
            for (int ii = 0; ii < wSupport; ii++) {
              float weight = hWeight[jj] * wWeight[ii];
              uint8_to_float(input, prevLevelMem +
                                        prevLevelPitch * (2 * j + jj) +
                                        4 * (2 * i + ii));
              result[0] += weight * input[0];
              result[1] += weight * input[1];
              result[2] += weight * input[2];
              result[3] += weight * input[3];
            }

          // convert back to format of the texture
          float_to_uint8(currLevelMem + currLevelPitch * j + 4 * i, result);
        }
      }
    }
  }
}

}
