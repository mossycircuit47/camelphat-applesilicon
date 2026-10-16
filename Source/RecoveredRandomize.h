#pragma once
#include "RecoveredFilters.h"
#include "ParameterSpec.h"
#include "RecoveredTables.h"
#include <cstdlib>
namespace camelphat_recovered {
struct Randomizer {
 std::array<float,53>& values;
 float uniform(float a=0,float b=1){float r=float(std::rand());float width=b-a;r=r*width;r=r*floatBits(0x30000000);return r+a;}
 int integer(int a,int b){float lo=float(a);float r=float(std::rand());float hi=float(b)+1.f;hi=hi-lo;r=r*hi;r=r*floatBits(0x30000000);lo=lo+r;return std::max(a,std::min(b,int(lo)));}
 float binary(float probability){return uniform()<probability?1.f:0.f;}
 float gaussian(float mean,float deviation,float minimum=0,float maximum=1){double x,y,r;do{x=double(std::rand());x=x+x;x=x-doubleBits(0x41dfffffffc00000ULL);x=x/doubleBits(0x41dfffffffc00000ULL);y=double(std::rand());y=y+y;y=y-doubleBits(0x41dfffffffc00000ULL);y=y/doubleBits(0x41dfffffffc00000ULL);r=x*x;double yy=y*y;r=r+yy;}while(r>1.0);double scale=std::log(r);scale=scale*(-2.0);scale=scale/r;scale=std::sqrt(scale);scale=double(float(scale));scale=scale*x;scale=scale*double(deviation);scale=double(mean)+scale;float out=float(scale);out=std::max(minimum,out);return std::min(maximum,out);}
 void set(int id,float v){auto& spec=camelphat::kParameterSpecs[size_t(id)];if(spec.minimum==0&&spec.maximum==1){values[size_t(id)]=v;return;}float width=spec.maximum-spec.minimum;float raw=v*width;raw=raw+spec.minimum;if(spec.originalType==0)raw=float(int(raw+.5f));raw=raw-spec.minimum;values[size_t(id)]=raw/width;}
 int destination(){int parameter=18;if(values[12]>.5f){if(values[0]>.5f)(void)binary(floatBits(0x3f333333));if(binary(.5f)==0){if(values[17]>0&&binary(floatBits(0x3e99999a))!=0)parameter=binary(.5f)==0?16:15;else {int choice=int(uniform(0,3));parameter=choice==0?17:(choice==1?13:(choice==2?19:18));}}}else {if(binary(.5f)==0){int choice=int(uniform(0,3));parameter=choice==0?3:(choice==1?4:(choice==2?5:18));}else parameter=binary(floatBits(0x3f333333))!=0?2:1;}int found=0;for(int i=0;i<23;++i)if(camelphat::modulationTargets[size_t(i)]==parameter)found=i;return found;}
 void run(){const float p3=floatBits(0x3e99999a),p7=floatBits(0x3f333333),p9=floatBits(0x3f666666),scale=floatBits(0x3c23d70a);
 if(values[0]>.5f){set(1,binary(p3)<.5f?0.f:gaussian(0,p3,0,floatBits(0x3f4ccccd)));set(2,binary(p3)==0?1.f:gaussian(1,p3,p3,1));set(3,uniform());set(4,uniform());set(5,gaussian(0,.5f));}
 if(values[7]>.5f){set(8,gaussian(0,.5f));set(9,gaussian(0,p3));set(10,gaussian(0,p7));set(11,gaussian(0,1));}
 if(values[12]>.5f){set(13,gaussian(1,.5f));set(14,uniform(0,float(integer(0,32))*scale));set(15,uniform());set(16,uniform());set(17,gaussian(0,.5f));set(18,uniform());set(19,gaussian(0,1.5f));}
 if(values[20]>.5f){set(21,uniform());set(22,uniform());}if(values[23]>.5f){set(24,uniform());set(25,uniform());}if(values[26]>.5f){set(27,uniform());set(28,uniform());}
 for(int on:{32,38})if(values[size_t(on)]>.5f){set(on+1,float(destination())*scale);set(on+2,uniform(0,float(integer(0,9))*scale));set(on+3,uniform());set(on+4,uniform());set(on+5,binary(p9));}
 int x=destination(),y=0,count=0;do{y=destination();++count;}while(x==y&&count<=49);set(44,float(x)*scale);set(45,float(y)*scale);
 }
};
inline void randomize(std::array<float,53>& values){Randomizer{values}.run();}
}
