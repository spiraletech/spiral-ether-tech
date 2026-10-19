#include "avatar/import/GlbMeshLoader.hpp"

#include <algorithm>
#include <cmath>
#include <cstring>
#include <fstream>
#include <limits>
#include <map>
#include <stdexcept>
#include <string_view>
#include <utility>

namespace hakui::import {
namespace {

// Minimal, strictly bounded JSON reader for the glTF metadata schema. Unlike
// substring searches, it handles nested arrays, strings and numeric tokens.
struct J {
    enum class Kind { Null, Bool, Number, String, Array, Object } kind = Kind::Null;
    double number = 0;
    std::string str;
    std::vector<J> array;
    std::map<std::string, J> object;
    const J& at(std::string_view key) const {
        static const J null{};
        auto it = object.find(std::string(key));
        return it == object.end() ? null : it->second;
    }
    const J& at(std::size_t i) const {
        static const J null{};
        return i >= array.size() ? null : array[i];
    }
    bool has(std::string_view key) const { return object.find(std::string(key)) != object.end(); }
    std::size_t size() const { return array.size(); }
    std::string text(std::string_view fallback = {}) const { return kind == Kind::String ? str : std::string(fallback); }
    int integer(int fallback = -1) const {
        if (kind != Kind::Number || !std::isfinite(number) || number < -2147483648. || number > 2147483647. || std::floor(number) != number) return fallback;
        return static_cast<int>(number);
    }
};

struct Parser {
    std::string_view s;
    std::size_t p = 0;
    void space() { while (p < s.size() && (s[p]==' ' || s[p]=='\t' || s[p]=='\r' || s[p]=='\n')) ++p; }
    void need(char c) { space(); if (p >= s.size() || s[p] != c) throw std::runtime_error("malformed JSON token"); ++p; }
    std::string string() {
        need('"'); std::string out;
        while (p < s.size()) {
            const unsigned char c = static_cast<unsigned char>(s[p++]);
            if (c == '"') return out;
            if (c < 0x20) throw std::runtime_error("control character in JSON string");
            if (c != '\\') { out.push_back(static_cast<char>(c)); continue; }
            if (p >= s.size()) throw std::runtime_error("truncated JSON escape");
            char e = s[p++];
            switch(e) {
                case '"': case '\\': case '/': out += e; break;
                case 'b': out += '\b'; break; case 'f': out += '\f'; break;
                case 'n': out += '\n'; break; case 'r': out += '\r'; break; case 't': out += '\t'; break;
                case 'u': {
                    if (p + 4 > s.size()) throw std::runtime_error("short unicode escape");
                    unsigned v = 0;
                    for (int k=0;k<4;++k) {
                        char h=s[p++]; v <<= 4;
                        if (h>='0'&&h<='9') v+=h-'0';
                        else if(h>='a'&&h<='f') v+=h-'a'+10;
                        else if(h>='A'&&h<='F') v+=h-'A'+10;
                        else throw std::runtime_error("invalid unicode escape");
                    }
                    if (v <= 0x7f) out.push_back(static_cast<char>(v));
                    else if (v <= 0x7ff) { out.push_back(static_cast<char>(0xc0 | (v>>6))); out.push_back(static_cast<char>(0x80 | (v&63))); }
                    else { out.push_back(static_cast<char>(0xe0 | (v>>12))); out.push_back(static_cast<char>(0x80 | ((v>>6)&63))); out.push_back(static_cast<char>(0x80 | (v&63))); }
                    break;
                }
                default: throw std::runtime_error("unknown JSON escape");
            }
            if (out.size() > 1'000'000) throw std::runtime_error("JSON string too long");
        }
        throw std::runtime_error("unterminated JSON string");
    }
    J value(int depth = 0) {
        if (depth > 64) throw std::runtime_error("JSON nesting limit");
        space(); if (p == s.size()) throw std::runtime_error("truncated JSON");
        J j;
        if (s[p]=='{') {
            j.kind=J::Kind::Object; ++p; space(); if(p<s.size()&&s[p]=='}'){++p;return j;}
            do { auto k=string(); need(':'); j.object.emplace(std::move(k),value(depth+1)); space();
                if(p==s.size()) throw std::runtime_error("unterminated JSON object");
                if(s[p]=='}'){++p;break;} need(',');
            } while(true);
        } else if (s[p]=='[') {
            j.kind=J::Kind::Array; ++p; space(); if(p<s.size()&&s[p]==']'){++p;return j;}
            do { j.array.push_back(value(depth+1)); if(j.array.size()>2'000'000) throw std::runtime_error("JSON array too long");
                space(); if(p==s.size()) throw std::runtime_error("unterminated JSON array");
                if(s[p]==']'){++p;break;} need(',');
            } while(true);
        } else if(s[p]=='"') { j.kind=J::Kind::String; j.str=string(); }
        else if(s.substr(p,4)=="true") {j.kind=J::Kind::Bool;j.number=1;p+=4;}
        else if(s.substr(p,5)=="false") {j.kind=J::Kind::Bool;p+=5;}
        else if(s.substr(p,4)=="null") {p+=4;}
        else {
            const auto start=p;
            if(s[p]=='-')++p;
            if(p>=s.size()) throw std::runtime_error("bad JSON number");
            if(s[p]=='0')++p;
            else {if(s[p]<'1'||s[p]>'9') throw std::runtime_error("bad JSON value");while(p<s.size()&&s[p]>='0'&&s[p]<='9')++p;}
            if(p<s.size()&&s[p]=='.'){++p; if(p==s.size()||s[p]<'0'||s[p]>'9')throw std::runtime_error("bad JSON decimal");while(p<s.size()&&s[p]>='0'&&s[p]<='9')++p;}
            if(p<s.size()&&(s[p]=='e'||s[p]=='E')){++p;if(p<s.size()&&(s[p]=='+'||s[p]=='-'))++p;if(p==s.size()||s[p]<'0'||s[p]>'9')throw std::runtime_error("bad JSON exponent");while(p<s.size()&&s[p]>='0'&&s[p]<='9')++p;}
            j.kind=J::Kind::Number; j.number=std::stod(std::string(s.substr(start,p-start)));
        }
        return j;
    }
};

std::uint32_t u32(const std::vector<std::uint8_t>& data, std::size_t off) {
    if(off > data.size() || data.size()-off < 4) throw std::runtime_error("truncated GLB u32");
    return std::uint32_t(data[off]) | (std::uint32_t(data[off+1])<<8) | (std::uint32_t(data[off+2])<<16) | (std::uint32_t(data[off+3])<<24);
}

std::array<float,16> identity(){return {1,0,0,0,0,1,0,0,0,0,1,0,0,0,0,1};}

struct Access {
    std::size_t count=0, components=0, size=0, stride=0, begin=0;
    int type=0; bool normalized=false;
};

struct Decoder {
    const J& root;
    const std::vector<std::uint8_t>& bin;
    Access accessor(int index) const {
        const J& a=root.at("accessors").at(static_cast<std::size_t>(index));
        if (index < 0 || a.kind!=J::Kind::Object) throw std::runtime_error("missing accessor");
        if(a.has("sparse"))throw std::runtime_error("sparse accessors not supported in M4A");
        const int viewIdx=a.at("bufferView").integer();
        const J& v=root.at("bufferViews").at(static_cast<std::size_t>(viewIdx));
        if(viewIdx<0 || v.kind!=J::Kind::Object || v.at("buffer").integer()!=0)throw std::runtime_error("invalid bufferView");
        Access out;
        out.count=static_cast<std::size_t>(a.at("count").integer());
        if(out.count==0 || out.count>2'000'000) throw std::runtime_error("accessor count out of bounds");
        out.type=a.at("componentType").integer();
        switch(out.type){case 5120:case 5121: out.size=1; break;case 5122:case 5123:out.size=2;break;case 5125:case 5126:out.size=4;break;default:throw std::runtime_error("unsupported glTF componentType");}
        const std::string shape=a.at("type").text();
        if(shape=="SCALAR")out.components=1;else if(shape=="VEC2")out.components=2;else if(shape=="VEC3")out.components=3;else if(shape=="VEC4")out.components=4;else if(shape=="MAT4")out.components=16;else throw std::runtime_error("unsupported glTF accessor shape");
        const int base=v.at("byteOffset").integer(0), local=a.at("byteOffset").integer(0), length=v.at("byteLength").integer(), step=v.at("byteStride").integer(static_cast<int>(out.components*out.size));
        if(base<0||local<0||length<=0||step < static_cast<int>(out.components*out.size))throw std::runtime_error("invalid accessor stride/offset");
        out.stride=static_cast<std::size_t>(step);
        out.begin=static_cast<std::size_t>(base)+static_cast<std::size_t>(local);
        const std::size_t elementBytes=out.components*out.size;
        if (static_cast<std::size_t>(local)>static_cast<std::size_t>(length) || (out.count-1) > (std::numeric_limits<std::size_t>::max()-elementBytes)/out.stride)throw std::runtime_error("accessor overflow");
        const std::size_t within=static_cast<std::size_t>(local)+(out.count-1)*out.stride+elementBytes;
        if(within>static_cast<std::size_t>(length) || out.begin>bin.size() || (out.count-1)*out.stride+elementBytes>bin.size()-out.begin)throw std::runtime_error("accessor outside binary buffer");
        out.normalized=a.at("normalized").kind==J::Kind::Bool && a.at("normalized").number!=0;
        return out;
    }
    double scalar(const Access& a, std::size_t row, std::size_t col) const {
        if(row>=a.count||col>=a.components) throw std::runtime_error("accessor index out of range");
        const std::size_t off=a.begin+row*a.stride+col*a.size;
        std::uint32_t raw=0; for(std::size_t i=0;i<a.size;++i)raw |= std::uint32_t(bin[off+i]) << (8*i);
        switch(a.type){
            case 5120: { auto v=static_cast<std::int8_t>(raw);return a.normalized?std::max(-1.,double(v)/127.):double(v); }
            case 5121: return a.normalized?double(raw)/255.:double(raw);
            case 5122: {auto v=static_cast<std::int16_t>(raw);return a.normalized?std::max(-1.,double(v)/32767.):double(v);}
            case 5123: return a.normalized?double(raw)/65535.:double(raw);
            case 5125: return double(raw);
            case 5126: {float f=0;std::memcpy(&f,&raw,4);if(!std::isfinite(f))throw std::runtime_error("nonfinite floating accessor");return double(f);}
            default:throw std::runtime_error("invalid accessor format");
        }
    }
    void check(const Access& a, std::size_t comps, std::initializer_list<int> types) const {
        if(a.components!=comps || std::find(types.begin(),types.end(),a.type)==types.end())throw std::runtime_error("incompatible attribute/accessor type");
    }
};

void load(const J& root, const std::vector<std::uint8_t>& bin, GlbMesh& dst){
    if(root.at("asset").at("version").text()!="2.0")throw std::runtime_error("GLB must declare glTF 2.0");
    if(root.at("buffers").size()!=1 || root.at("buffers").at(0).has("uri"))throw std::runtime_error("only embedded GLB buffers supported");
    const int declared=root.at("buffers").at(0).at("byteLength").integer();
    if(declared<0 || static_cast<std::size_t>(declared)>bin.size())throw std::runtime_error("GLB embedded buffer length mismatch");
    Decoder d{root,bin};
    const J& skins=root.at("skins"), nodes=root.at("nodes"), meshes=root.at("meshes");
    if(skins.size()!=1 || nodes.size()==0)throw std::runtime_error("M4A requires exactly one skin and node hierarchy");
    const J& joints=skins.at(0).at("joints");
    if(joints.size()<1 || joints.size()>256)throw std::runtime_error("skin joints count out of bounds");
    std::map<int,int> nodeToJoint;
    for(std::size_t i=0;i<joints.size();++i){int n=joints.at(i).integer();if(n<0||static_cast<std::size_t>(n)>=nodes.size()||nodeToJoint.count(n))throw std::runtime_error("invalid/duplicate joint node");nodeToJoint[n]=static_cast<int>(i);}
    std::vector<int> parents(nodes.size(),-1);
    for(std::size_t n=0;n<nodes.size();++n){for(const J& c:nodes.at(n).at("children").array){int k=c.integer();if(k<0||static_cast<std::size_t>(k)>=nodes.size()||parents[k]!=-1)throw std::runtime_error("invalid node hierarchy");parents[k]=static_cast<int>(n);}}
    const int inverseAcc=skins.at(0).at("inverseBindMatrices").integer();
    Access inverses; if(inverseAcc>=0){inverses=d.accessor(inverseAcc);d.check(inverses,16,{5126});if(inverses.count!=joints.size())throw std::runtime_error("inverse bind matrix count differs from joints");}
    for(std::size_t i=0;i<joints.size();++i){const int n=joints.at(i).integer();Joint j;j.name=nodes.at(n).at("name").text("Joint_"+std::to_string(i));j.inverseBind=identity();
        const J& jointNode=nodes.at(n);
        if(jointNode.has("matrix")) throw std::runtime_error("joint matrix transforms require TRS decomposition (unsupported M4A)");
        for(int k=0;k<3;++k){
            if(jointNode.has("translation")) j.translation[k]=static_cast<float>(jointNode.at("translation").at(k).number);
            if(jointNode.has("scale")) j.scale[k]=static_cast<float>(jointNode.at("scale").at(k).number);
        }
        if(jointNode.has("rotation"))for(int k=0;k<4;++k)j.rotation[k]=static_cast<float>(jointNode.at("rotation").at(k).number);
        if(inverseAcc>=0)for(int k=0;k<16;++k)j.inverseBind[k]=static_cast<float>(d.scalar(inverses,i,k));
        for(int p=parents[n],depth=0;p>=0 && depth<64;p=parents[p],++depth){auto it=nodeToJoint.find(p);if(it!=nodeToJoint.end()){j.parent=it->second;break;}}
        dst.joints.push_back(std::move(j));
    }
    // The input joints can be unordered; consumers must topologically sort during FK.
    for(std::size_t n=0;n<nodes.size();++n){
        const J& node=nodes.at(n);if(node.at("skin").integer()!=0 || node.at("mesh").integer()<0)continue;
        const int mi=node.at("mesh").integer();if(static_cast<std::size_t>(mi)>=meshes.size())throw std::runtime_error("skinned node mesh index invalid");
        if(node.has("matrix")||node.has("translation")||node.has("rotation")||node.has("scale"))throw std::runtime_error("nonidentity skinned mesh node transforms not supported in M4A");
        const J& mesh=meshes.at(mi);const J& prims=mesh.at("primitives");
        for(std::size_t pi=0;pi<prims.size();++pi){
            const J& prim=prims.at(pi);const J& attrs=prim.at("attributes");
            if(prim.at("mode").integer(4)!=4)throw std::runtime_error("only TRIANGLES mode supported");
            const int posIdx=attrs.at("POSITION").integer(), jIdx=attrs.at("JOINTS_0").integer(),wIdx=attrs.at("WEIGHTS_0").integer();
            if(posIdx<0||jIdx<0||wIdx<0)throw std::runtime_error("skinned primitive missing POSITION/JOINTS_0/WEIGHTS_0");
            const Access pos=d.accessor(posIdx), joints0=d.accessor(jIdx), weights0=d.accessor(wIdx);
            d.check(pos,3,{5126});d.check(joints0,4,{5121,5123});d.check(weights0,4,{5121,5123,5126});
            if(pos.count!=joints0.count||pos.count!=weights0.count)throw std::runtime_error("attribute vertex counts differ");
            const bool hasNormals=attrs.at("NORMAL").integer()>=0, hasUv=attrs.at("TEXCOORD_0").integer()>=0;
            Access norm,uv;if(hasNormals){norm=d.accessor(attrs.at("NORMAL").integer());d.check(norm,3,{5126});if(norm.count!=pos.count)throw std::runtime_error("normal vertex count mismatch");}if(hasUv){uv=d.accessor(attrs.at("TEXCOORD_0").integer());d.check(uv,2,{5126,5121,5123});if(uv.count!=pos.count)throw std::runtime_error("UV count mismatch");}
            if(dst.vertices.size()+pos.count>2'000'000)throw std::runtime_error("vertex safety limit");
            const std::uint32_t base=static_cast<std::uint32_t>(dst.vertices.size());
            for(std::size_t v=0;v<pos.count;++v){SkinnedVertex vertex;float total=0;
                for(int k=0;k<3;++k){vertex.position[k]=static_cast<float>(d.scalar(pos,v,k));if(hasNormals)vertex.normal[k]=static_cast<float>(d.scalar(norm,v,k));}
                if(hasUv)for(int k=0;k<2;++k)vertex.uv[k]=static_cast<float>(d.scalar(uv,v,k));
                for(int k=0;k<4;++k){int idx=static_cast<int>(d.scalar(joints0,v,k));if(idx<0||idx>=static_cast<int>(dst.joints.size()))throw std::runtime_error("vertex references nonexistent skin joint");vertex.joints[k]=static_cast<std::uint16_t>(idx);vertex.weights[k]=static_cast<float>(d.scalar(weights0,v,k));if(!std::isfinite(vertex.weights[k])||vertex.weights[k]<0)throw std::runtime_error("invalid vertex weight");total+=vertex.weights[k];}
                if(total<1e-6f) throw std::runtime_error("vertex has zero skin weights");
                for(float& w:vertex.weights) w/=total;
                dst.vertices.push_back(vertex);
            }
            Submesh sub;sub.name=mesh.at("name").text("mesh_"+std::to_string(mi))+"_"+std::to_string(pi);sub.firstIndex=static_cast<std::uint32_t>(dst.indices.size());const int materialIdx=prim.at("material").integer();if(materialIdx>=0)sub.material=root.at("materials").at(static_cast<std::size_t>(materialIdx)).at("name").text();
            const int ia=prim.at("indices").integer();if(ia>=0){const Access ix=d.accessor(ia);d.check(ix,1,{5121,5123,5125});sub.indexCount=static_cast<std::uint32_t>(ix.count);for(std::size_t q=0;q<ix.count;++q){std::uint32_t k=static_cast<std::uint32_t>(d.scalar(ix,q,0));if(k>=pos.count)throw std::runtime_error("triangle index out of bounds");dst.indices.push_back(base+k);}}
            else {sub.indexCount=static_cast<std::uint32_t>(pos.count);for(std::uint32_t q=0;q<pos.count;++q)dst.indices.push_back(base+q);}
            if(sub.indexCount%3) throw std::runtime_error("triangle indices not divisible by three");
            dst.submeshes.push_back(std::move(sub));
            const J& targets=prim.at("targets");const J& names=mesh.at("extras").at("targetNames");
            while(dst.morphTargets.size()<targets.size()){MorphTarget m;const std::size_t k=dst.morphTargets.size();m.name=names.at(k).text("Morph_"+std::to_string(k));m.positionDeltas.resize(base);dst.morphTargets.push_back(std::move(m));}
            for(auto& t:dst.morphTargets)t.positionDeltas.resize(dst.vertices.size());
            for(std::size_t ti=0;ti<targets.size();++ti){int idx=targets.at(ti).at("POSITION").integer();if(idx<0)continue;Access delta=d.accessor(idx);d.check(delta,3,{5126});if(delta.count!=pos.count)throw std::runtime_error("morph target count mismatch");for(std::size_t v=0;v<pos.count;++v)for(int k=0;k<3;++k)dst.morphTargets[ti].positionDeltas[base+v][k]=static_cast<float>(d.scalar(delta,v,k));}
        }
    }
    if(dst.vertices.empty()||dst.indices.empty())throw std::runtime_error("no skinned mesh nodes found");
    const J& animations=root.at("animations");
    for(std::size_t ai=0;ai<animations.size();++ai){const J& animation=animations.at(ai), samplers=animation.at("samplers");const std::string name=animation.at("name").text("clip_"+std::to_string(ai));
        for(const J& ch:animation.at("channels").array){const int node=ch.at("target").at("node").integer(),samplerIdx=ch.at("sampler").integer();auto it=nodeToJoint.find(node);if(it==nodeToJoint.end())continue;
            if(samplerIdx<0||static_cast<std::size_t>(samplerIdx)>=samplers.size()) throw std::runtime_error("animation sampler invalid");
            const J& sample=samplers.at(samplerIdx);
            const std::string path=ch.at("target").at("path").text(),interpolation=sample.at("interpolation").text("LINEAR");
            if(path!="translation"&&path!="rotation"&&path!="scale")continue;
            if(interpolation!="LINEAR"&&interpolation!="STEP")throw std::runtime_error("CUBICSPLINE animations not supported in M4A");
            Access times=d.accessor(sample.at("input").integer()), values=d.accessor(sample.at("output").integer());d.check(times,1,{5126});d.check(values,path=="rotation"?4:3,{5126});if(times.count!=values.count)throw std::runtime_error("animation keyframe count mismatch");
            AnimationChannel channel;channel.clip=name;channel.joint=static_cast<std::uint32_t>(it->second);channel.path=path;channel.interpolation=interpolation;
            float last=-std::numeric_limits<float>::infinity();for(std::size_t k=0;k<times.count;++k){float t=static_cast<float>(d.scalar(times,k,0));if(t<0 || t<=last)throw std::runtime_error("animation timestamps not increasing");last=t;channel.times.push_back(t);std::array<float,4> out{};for(std::size_t c=0;c<values.components;++c)out[c]=static_cast<float>(d.scalar(values,k,c));channel.values.push_back(out);}
            dst.animationChannels.push_back(std::move(channel));
        }
    }
}

} // namespace

LoadResult loadGlb(const std::filesystem::path& path) {
    LoadResult out;
    try {
        std::ifstream file(path,std::ios::binary|std::ios::ate);
        if(!file)throw std::runtime_error("cannot open GLB file");
        const std::streamoff length=file.tellg();
        if(length<20||length>256*1024*1024)throw std::runtime_error("invalid GLB file size");
        file.seekg(0);std::vector<std::uint8_t> data(static_cast<std::size_t>(length));
        if(!file.read(reinterpret_cast<char*>(data.data()),length))throw std::runtime_error("GLB read failure");
        if(u32(data,0)!=0x46546c67||u32(data,4)!=2||u32(data,8)!=data.size())throw std::runtime_error("invalid glTF 2.0 binary header/length");
        std::size_t off=12;std::string_view json;std::vector<std::uint8_t> bin;
        if(u32(data,off+4)!=0x4e4f534a)throw std::runtime_error("first GLB chunk must be JSON");
        while(off<data.size()){
            if(data.size()-off<8)throw std::runtime_error("truncated GLB chunk header");
            const auto size=u32(data,off),type=u32(data,off+4);off+=8;
            if(size%4||size>data.size()-off)throw std::runtime_error("invalid GLB chunk size");
            if(type==0x4e4f534a){if(!json.empty())throw std::runtime_error("duplicate JSON chunk");json=std::string_view(reinterpret_cast<const char*>(data.data()+off),size);}
            if(type==0x004e4942){if(!bin.empty())throw std::runtime_error("duplicate BIN chunk");bin.assign(data.begin()+static_cast<std::ptrdiff_t>(off),data.begin()+static_cast<std::ptrdiff_t>(off+size));}
            off+=size;
        }
        if(json.empty()||bin.empty())throw std::runtime_error("GLB requires embedded JSON and BIN chunks");
        Parser parser{json};J root=parser.value();parser.space();if(parser.p!=json.size()&&json.substr(parser.p).find_first_not_of(' ' )!=std::string_view::npos)throw std::runtime_error("trailing JSON data");
        load(root,bin,out.mesh);
        out.ok=true;
    } catch(const std::exception& e){out.ok=false;out.error=e.what();out.mesh={};}
    return out;
}
} // namespace hakui::import
