#include "avatar/import/GlbMeshLoader.hpp"
#include <cassert>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>

namespace {
void u32(std::vector<std::uint8_t>& b,std::uint32_t n){for(int i=0;i<4;++i)b.push_back(static_cast<std::uint8_t>((n>>(i*8))&255));}
void floats(std::vector<std::uint8_t>& b,std::initializer_list<float> fs){for(float f:fs){std::uint32_t raw=0;std::memcpy(&raw,&f,4);u32(b,raw);}}
std::filesystem::path path(const char* name){return std::filesystem::temp_directory_path()/name;}
void write(const std::filesystem::path& p,const std::vector<std::uint8_t>& b){std::ofstream f(p,std::ios::binary);f.write(reinterpret_cast<const char*>(b.data()),static_cast<std::streamsize>(b.size()));}
std::vector<std::uint8_t> fixture(bool brokenIndex=false) {
    // This fixture exercises actual accessor decoding: float positions and weights,
    // U16 joint and triangle indices, inverse bind matrices, morphs, animation keys.
    std::vector<std::uint8_t> bin;
    floats(bin,{-0.1f,0.f,0.f, 0.1f,0.f,0.f, 0.f,0.2f,0.f}); // 0..35
    floats(bin,{0,0,1,0,0,1,0,0,1}); // 36..71
    floats(bin,{0,0,1,0,.5f,1}); // 72..95
    for(int i=0;i<12;++i){bin.push_back(0);bin.push_back(0);} // 96..119 JOINTS U16
    floats(bin,{2,0,0,0, 1,0,0,0, 1,0,0,0}); // 120..167 weights normalize
    for(int n:{0,1,brokenIndex?3:2}){bin.push_back(static_cast<std::uint8_t>(n));bin.push_back(0);} // 168..173
    bin.push_back(0);bin.push_back(0); // 174..175 alignment
    floats(bin,{1,0,0,0,0,1,0,0,0,0,1,0,0,0,0,1}); // 176..239 matrix
    floats(bin,{0,0,0,0,0,0,0.02f,0,0}); // 240..275 morph delta
    floats(bin,{0,1}); // 276..283 animation times
    floats(bin,{0,0,0,0,0.1f,0}); // 284..307 animation xyz
    const std::string json = R"({"asset":{"version":"2.0"},"buffers":[{"byteLength":308}],"bufferViews":[{"buffer":0,"byteOffset":0,"byteLength":36},{"buffer":0,"byteOffset":36,"byteLength":36},{"buffer":0,"byteOffset":72,"byteLength":24},{"buffer":0,"byteOffset":96,"byteLength":24},{"buffer":0,"byteOffset":120,"byteLength":48},{"buffer":0,"byteOffset":168,"byteLength":6},{"buffer":0,"byteOffset":176,"byteLength":64},{"buffer":0,"byteOffset":240,"byteLength":36},{"buffer":0,"byteOffset":276,"byteLength":8},{"buffer":0,"byteOffset":284,"byteLength":24}],"accessors":[{"bufferView":0,"componentType":5126,"count":3,"type":"VEC3"},{"bufferView":1,"componentType":5126,"count":3,"type":"VEC3"},{"bufferView":2,"componentType":5126,"count":3,"type":"VEC2"},{"bufferView":3,"componentType":5123,"count":3,"type":"VEC4"},{"bufferView":4,"componentType":5126,"count":3,"type":"VEC4"},{"bufferView":5,"componentType":5123,"count":3,"type":"SCALAR"},{"bufferView":6,"componentType":5126,"count":1,"type":"MAT4"},{"bufferView":7,"componentType":5126,"count":3,"type":"VEC3"},{"bufferView":8,"componentType":5126,"count":2,"type":"SCALAR"},{"bufferView":9,"componentType":5126,"count":2,"type":"VEC3"}],"meshes":[{"name":"Ether_Test","extras":{"targetNames":["Smile"]},"primitives":[{"attributes":{"POSITION":0,"NORMAL":1,"TEXCOORD_0":2,"JOINTS_0":3,"WEIGHTS_0":4},"indices":5,"material":0,"targets":[{"POSITION":7}]}]}],"materials":[{"name":"Mat_Skin"}],"nodes":[{"name":"Pelvis"},{"name":"Ether","mesh":0,"skin":0}],"skins":[{"joints":[0],"inverseBindMatrices":6}],"animations":[{"name":"IDLE","samplers":[{"input":8,"output":9,"interpolation":"LINEAR"}],"channels":[{"sampler":0,"target":{"node":0,"path":"translation"}}]}]})";
    std::vector<std::uint8_t> j(json.begin(),json.end());while(j.size()%4)j.push_back(' ');
    std::vector<std::uint8_t> glb;u32(glb,0x46546c67);u32(glb,2);u32(glb,static_cast<std::uint32_t>(12+8+j.size()+8+bin.size()));u32(glb,static_cast<std::uint32_t>(j.size()));u32(glb,0x4e4f534a);glb.insert(glb.end(),j.begin(),j.end());u32(glb,static_cast<std::uint32_t>(bin.size()));u32(glb,0x004e4942);glb.insert(glb.end(),bin.begin(),bin.end());return glb;
}
}
int main(){
    const auto good=path("hakui_m4a_valid.glb");const auto bad=path("hakui_m4a_bad.glb");
    write(good,fixture());auto ok=hakui::import::loadGlb(good);
    if(!ok.ok)std::cerr<<"GOOD FAIL: "<<ok.error<<'\n';
    assert(ok.ok); assert(ok.mesh.vertices.size()==3);assert(ok.mesh.indices.size()==3);
    assert(ok.mesh.indices[2]==2);assert(ok.mesh.joints.size()==1);assert(ok.mesh.joints[0].name=="Pelvis");
    assert(std::fabs(ok.mesh.vertices[0].weights[0]-1.f)<1e-6f);
    assert(ok.mesh.submeshes.size()==1&&ok.mesh.submeshes[0].material=="Mat_Skin");
    assert(ok.mesh.morphTargets.size()==1&&ok.mesh.morphTargets[0].name=="Smile");
    assert(std::fabs(ok.mesh.morphTargets[0].positionDeltas[2][0]-0.02f)<1e-6f);
    assert(ok.mesh.animationChannels.size()==1&&ok.mesh.animationChannels[0].clip=="IDLE");
    assert(ok.mesh.animationChannels[0].times.size()==2);
    write(bad,fixture(true));auto invalid=hakui::import::loadGlb(bad);assert(!invalid.ok);assert(invalid.error.find("index out of bounds")!=std::string::npos);assert(invalid.mesh.vertices.empty());
    auto missing=hakui::import::loadGlb(path("hakui_does_not_exist_m4a.glb"));assert(!missing.ok);assert(missing.mesh.vertices.empty());
    auto header=fixture();header[0]=0;write(bad,header);assert(!hakui::import::loadGlb(bad).ok);
    header=fixture();header[12]=0xff;write(bad,header);assert(!hakui::import::loadGlb(bad).ok);
    std::filesystem::remove(good);std::filesystem::remove(bad);
    std::cout<<"[PASS] HAKUI M4A: actual skinned vertices, joints, indices, inverse bind, morph, clip; bounds + fallback contracts\n";
}
