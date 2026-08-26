#pragma once
#include "engine/ingest/observation.hpp"
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <deque>
#include <vector>
namespace mf {
struct OccupancyBlob {
    int id=0; float x=0,z=0,vx=0,vz=0,rx=0.6f,rz=0.4f,angle=0,energy=0,motion=0,age=0;
    std::vector<float> contour, trail;
};
struct CsiEstimate {
    int gw=28,gz=28; std::vector<float> grid; std::vector<OccupancyBlob> blobs;
    float motion=0, energy=0; bool live=false;
};
class CsiInferencer {
public:
    void push(const FieldObservation& obs){
        if(!obs.valid||obs.csi.empty()) return;
        Frame f; f.csi=obs.csi; f.energy=obs.region("csi_energy"); f.synthetic=obs.synthetic;
        if(hist_.size()>=kMax) hist_.pop_front();
        hist_.push_back(std::move(f)); rebuild();
    }
    CsiEstimate estimate() const { return last_; }
private:
    static constexpr size_t kMax=64; static constexpr float kWorld=6.5f;
    static constexpr int kGW=28,kGZ=28;
    struct Frame{ std::vector<float> csi; float energy=0; bool synthetic=true; };
    struct Track{ OccupancyBlob blob; int missed=0; };
    std::deque<Frame> hist_; std::vector<float> ema_; std::vector<Track> tracks_; int next_id_=1; CsiEstimate last_;
    void rebuild(){
        CsiEstimate e; e.gw=kGW; e.gz=kGZ; e.grid.assign(size_t(kGW*kGZ),0.f);
        if(hist_.empty()){ last_=std::move(e); return; }
        const Frame& cur=hist_.back(); e.live=!cur.synthetic; e.energy=cur.energy;
        const Frame* prev=hist_.size()>1?&hist_[hist_.size()-2]:nullptr;
        float motion_acc=0; int motion_n=0; const int n=int(cur.csi.size());
        for(int i=0;i<n;++i){
            const float amp=std::clamp(cur.csi[size_t(i)],0.f,1.f); float d=0;
            if(prev&&i<int(prev->csi.size())){ d=std::fabs(amp-prev->csi[size_t(i)]); motion_acc+=d; ++motion_n; }
            const float theta=(float(i)/std::max(1,n-1)-0.5f)*6.2831853f;
            const float range=0.55f+(1.f-amp)*3.35f;
            splat(e.grid,e.gw,e.gz,std::cos(theta)*range,std::sin(theta)*range,0.28f+amp*0.7f,0.48f);
            if(d>0.03f){ const float mr=0.9f+(1.f-d)*2.6f; splat(e.grid,e.gw,e.gz,std::cos(theta)*mr,std::sin(theta)*mr,0.2f+d*1.1f,0.38f); }
        }
        e.motion=motion_n?motion_acc/float(motion_n):0.f;
        if(ema_.size()!=e.grid.size()) ema_.assign(e.grid.size(),0.f);
        for(size_t i=0;i<e.grid.size();++i) ema_[i]=ema_[i]*0.72f+e.grid[i]*0.28f;
        e.grid=ema_;
        auto raw=extract_raw(e); match_tracks(raw,e.motion);
        e.blobs.clear(); for(auto& t:tracks_) e.blobs.push_back(t.blob); last_=std::move(e);
    }
    static void splat(std::vector<float>& grid,int gw,int gz,float x,float z,float amp,float sigma){
        const float half=kWorld*0.5f,sx=float(gw)/kWorld,sz=float(gz)/kWorld;
        const int cx=int((x+half)*sx), cz=int((z+half)*sz), rad=std::max(1,int(sigma*sx*2.2f));
        const float inv=1.f/(2.f*sigma*sigma);
        for(int jz=cz-rad;jz<=cz+rad;++jz){ if(jz<0||jz>=gz) continue;
            for(int ix=cx-rad;ix<=cx+rad;++ix){ if(ix<0||ix>=gw) continue;
                const float px=(float(ix)+0.5f)/sx-half, pz=(float(jz)+0.5f)/sz-half;
                const float dx=px-x, dz=pz-z;
                grid[size_t(jz*gw+ix)]+=amp*std::exp(-(dx*dx+dz*dz)*inv);
            } } }
    static void fill_contour(OccupancyBlob& b){
        b.contour.clear(); b.contour.reserve(48);
        for(int i=0;i<24;++i){ const float t=float(i)/24.f*6.2831853f,ct=std::cos(t),st=std::sin(t),ca=std::cos(b.angle),sa=std::sin(b.angle);
            const float lx=ct*b.rx,lz=st*b.rz; b.contour.push_back(b.x+ca*lx-sa*lz); b.contour.push_back(b.z+sa*lx+ca*lz); } }
    std::vector<OccupancyBlob> extract_raw(const CsiEstimate& e) const {
        std::vector<OccupancyBlob> out; float peak=0; for(float v:e.grid) peak=std::max(peak,v); if(peak<1e-4f) return out;
        const float thr=peak*0.40f; const int gw=e.gw,gz=e.gz; std::vector<char> used(e.grid.size(),0);
        auto idx=[&](int x,int z){return z*gw+x;};
        for(int seed=0;seed<4;++seed){
            int bx=-1,bz=-1; float best=thr;
            for(int z=1;z<gz-1;++z) for(int x=1;x<gw-1;++x){ if(used[size_t(idx(x,z))]) continue; float v=e.grid[size_t(idx(x,z))]; if(v>best){best=v;bx=x;bz=z;} }
            if(bx<0) break;
            OccupancyBlob b; float sx=0,sz=0,sw=0,sxx=0,szz=0,sxz=0; const float rthr=peak*0.38f;
            std::vector<std::pair<int,int>> q{{bx,bz}}; used[size_t(idx(bx,bz))]=1;
            for(size_t qi=0;qi<q.size();++qi){ auto [x,z]=q[qi]; float w=e.grid[size_t(idx(x,z))];
                float px=(float(x)+0.5f)/float(gw)*kWorld-kWorld*0.5f, pz=(float(z)+0.5f)/float(gz)*kWorld-kWorld*0.5f;
                sx+=px*w; sz+=pz*w; sw+=w; sxx+=px*px*w; szz+=pz*pz*w; sxz+=px*pz*w;
                for(int dz=-1;dz<=1;++dz) for(int dx=-1;dx<=1;++dx){ if(!dx&&!dz) continue; int nx=x+dx,nz=z+dz;
                    if(nx<0||nz<0||nx>=gw||nz>=gz) continue; size_t ii=size_t(idx(nx,nz)); if(used[ii]||e.grid[ii]<rthr) continue; used[ii]=1; q.push_back({nx,nz}); } }
            if(sw<1e-5f) continue;
            b.x=sx/sw; b.z=sz/sw; b.energy=std::min(1.f,best/std::max(peak,1e-4f));
            float varx=std::max(0.04f,sxx/sw-b.x*b.x), varz=std::max(0.04f,szz/sw-b.z*b.z), cov=sxz/sw-b.x*b.z;
            b.rx=std::clamp(std::sqrt(varx)*2.f,0.22f,2.f); b.rz=std::clamp(std::sqrt(varz)*2.f,0.22f,2.f);
            b.angle=0.5f*std::atan2(2.f*cov,varx-varz); fill_contour(b); if(b.energy>0.05f) out.push_back(std::move(b));
        } return out;
    }
    void match_tracks(const std::vector<OccupancyBlob>& raw,float motion){
        std::vector<char> taken(raw.size(),0);
        for(auto& t:tracks_){
            int best=-1; float best_d=1.15f;
            for(size_t i=0;i<raw.size();++i){ if(taken[i]) continue; float dx=raw[i].x-t.blob.x, dz=raw[i].z-t.blob.z, d=std::sqrt(dx*dx+dz*dz); if(d<best_d){best_d=d;best=int(i);} }
            if(best>=0){ taken[size_t(best)]=1; const auto& r=raw[size_t(best)];
                float nx=t.blob.x*0.55f+r.x*0.45f, nz=t.blob.z*0.55f+r.z*0.45f;
                t.blob.vx=(nx-t.blob.x)*12.f; t.blob.vz=(nz-t.blob.z)*12.f; t.blob.x=nx; t.blob.z=nz;
                t.blob.rx=t.blob.rx*0.6f+r.rx*0.4f; t.blob.rz=t.blob.rz*0.6f+r.rz*0.4f; t.blob.angle=r.angle;
                t.blob.energy=t.blob.energy*0.5f+r.energy*0.5f; t.blob.motion=motion; t.blob.age+=0.08f; t.missed=0; fill_contour(t.blob);
                t.blob.trail.push_back(t.blob.x); t.blob.trail.push_back(t.blob.z);
                if(t.blob.trail.size()>36) t.blob.trail.erase(t.blob.trail.begin(), t.blob.trail.begin()+2);
            } else t.missed++;
        }
        tracks_.erase(std::remove_if(tracks_.begin(),tracks_.end(),[](const Track& t){return t.missed>8;}), tracks_.end());
        for(size_t i=0;i<raw.size();++i){ if(taken[i]) continue; Track t; t.blob=raw[i]; t.blob.id=next_id_++; t.blob.motion=motion; t.blob.trail={t.blob.x,t.blob.z}; tracks_.push_back(std::move(t)); }
    }
};
} // namespace mf
