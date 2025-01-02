#include "TextureHelper.hpp"

#include "triglav/graphics_api/Texture.hpp"

namespace triglav::graphics_api {

void copy_texture(CommandList& cmdList, const Texture& src, const Texture& dst)
{
   static constexpr auto expectedTextureState = TextureState::ShaderRead;

   // const TextureBarrierInfo barrierSrcIn{
   //    .texture = &src,
   //    .sourceState = expectedTextureState,
   //    .targetState = TextureState::TransferSrc,
   //    .baseMipLevel = 0,
   //    .mipLevelCount = 1,
   // };
   // cmdList.texture_barrier(PipelineStage::FragmentShader, PipelineStage::Transfer, barrierSrcIn);

   const TextureBarrierInfo barrierDstIn{
      .texture = &dst,
      .sourceState = TextureState::Undefined,
      .targetState = TextureState::TransferDst,
      .baseMipLevel = 0,
      .mipLevelCount = 1,
   };
   cmdList.texture_barrier(PipelineStage::Transfer, PipelineStage::Transfer, barrierDstIn);

   cmdList.copy_texture(src, TextureState::TransferSrc, dst, TextureState::TransferDst);

   const TextureBarrierInfo barrierDstOut{
      .texture = &dst,
      .sourceState = TextureState::TransferDst,
      .targetState = expectedTextureState,
      .baseMipLevel = 0,
      .mipLevelCount = 1,
   };
   cmdList.texture_barrier(PipelineStage::Transfer, PipelineStage::FragmentShader, barrierDstOut);

   // const TextureBarrierInfo barrierSrcOut{
   //    .texture = &src,
   //    .sourceState = TextureState::TransferSrc,
   //    .targetState = expectedTextureState,
   //    .baseMipLevel = 0,
   //    .mipLevelCount = 1,
   // };
   // cmdList.texture_barrier(PipelineStage::Transfer, PipelineStage::FragmentShader, barrierSrcOut);
}


}// namespace triglav::graphics_api
