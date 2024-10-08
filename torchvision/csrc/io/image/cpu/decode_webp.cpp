#include "decode_webp.h"

#if WEBP_FOUND
#include "webp/decode.h"
#endif // WEBP_FOUND

namespace vision {
namespace image {

#if !WEBP_FOUND
torch::Tensor decode_webp(
    const torch::Tensor& encoded_data,
    ImageReadMode mode) {
  TORCH_CHECK(
      false, "decode_webp: torchvision not compiled with libwebp support");
}
#else

torch::Tensor decode_webp(
    const torch::Tensor& encoded_data,
    ImageReadMode mode) {
  TORCH_CHECK(encoded_data.is_contiguous(), "Input tensor must be contiguous.");
  TORCH_CHECK(
      encoded_data.dtype() == torch::kU8,
      "Input tensor must have uint8 data type, got ",
      encoded_data.dtype());
  TORCH_CHECK(
      encoded_data.dim() == 1,
      "Input tensor must be 1-dimensional, got ",
      encoded_data.dim(),
      " dims.");

  auto encoded_data_p = encoded_data.data_ptr<uint8_t>();
  auto encoded_data_size = encoded_data.numel();

  WebPBitstreamFeatures features;
  auto res = WebPGetFeatures(encoded_data_p, encoded_data_size, &features);
  TORCH_CHECK(
      res == VP8_STATUS_OK, "WebPGetFeatures failed with error code ", res);
  TORCH_CHECK(
      !features.has_animation, "Animated webp files are not supported.");

  if (mode != IMAGE_READ_MODE_UNCHANGED && mode != IMAGE_READ_MODE_RGB &&
      mode != IMAGE_READ_MODE_RGB_ALPHA) {
    // Other modes aren't supported, but we don't error or even warn because we
    // have generic entry points like decode_image which may support all modes,
    // it just depends on the underlying decoder.
    mode = IMAGE_READ_MODE_UNCHANGED;
  }

  // If return_rgb is false it means we return rgba - nothing else.
  auto return_rgb =
      (mode == IMAGE_READ_MODE_RGB ||
       (mode == IMAGE_READ_MODE_UNCHANGED && !features.has_alpha));

  auto decoding_func = return_rgb ? WebPDecodeRGB : WebPDecodeRGBA;
  auto num_channels = return_rgb ? 3 : 4;

  int width = 0;
  int height = 0;

  auto decoded_data =
      decoding_func(encoded_data_p, encoded_data_size, &width, &height);
  TORCH_CHECK(decoded_data != nullptr, "WebPDecodeRGB[A] failed.");

  auto out = torch::from_blob(
      decoded_data, {height, width, num_channels}, torch::kUInt8);

  return out.permute({2, 0, 1});
}
#endif // WEBP_FOUND

} // namespace image
} // namespace vision
