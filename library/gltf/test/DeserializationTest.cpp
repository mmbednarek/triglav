#include "TestData.hpp"

#include "triglav/gltf/BufferManager.hpp"
#include "triglav/gltf/Gltf.hpp"
#include "triglav/gltf/MeshLoad.hpp"
#include "triglav/io/StringReader.hpp"

#include <gtest/gtest.h>

TEST(DerserializationTest, BasicDeserialization)
{
   triglav::io::StringReader reader(triglav::test::g_gltfExample);

   triglav::gltf::Document doc;
   doc.deserialize(reader);

   EXPECT_EQ(doc.scene, 0u);
   EXPECT_EQ(doc.asset.generator, "COLLADA2GLTF");
   EXPECT_EQ(doc.asset.version, "2.0");
}

TEST(DerserializationTest, ReadMeshTest)
{
   triglav::io::StringReader reader(triglav::test::g_gltfExample);

   triglav::gltf::Document doc;
   doc.deserialize(reader);

   triglav::gltf::BufferManager manager(doc);

   auto mesh = triglav::gltf::mesh_from_document(doc, 0, manager);
}

TEST(DerserializationTest, ReadGlb)
{
   auto mesh = triglav::gltf::load_glb_mesh(triglav::io::Path{"example.glb"});
}
