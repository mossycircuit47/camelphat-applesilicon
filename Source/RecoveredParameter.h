#pragma once
#include "ParameterSpec.h"
#include <algorithm>
namespace camelphat_recovered {
struct Parameter {
 float current=0,stored=0,start=0,minimum=0,maximum=1;
 int type=1,elapsed=0,smoothingFrames=441;
 void configure(const camelphat::ParameterSpec& s,float sr) {minimum=s.minimum;maximum=s.maximum;type=s.originalType;smoothingFrames=int(.01f*sr);current=stored=minimum;}
 void set(float normalized) {
  // Original comparison is against the raw stored value, before conversion.
  if(normalized==stored)return;
  float v=normalized;
  if(minimum!=0 || maximum!=1){v=v*(maximum-minimum);v=v+minimum;}
  if(type==0)v=float(int(v+.5f));
  stored=v;elapsed=0;start=current;
 }
 void reset(){current=stored;elapsed=0;start=0;}
 void advance(int n){if(current==stored)return;elapsed+=n;float fraction=float(elapsed)/float(smoothingFrames);float remainder=1.f-fraction;if(fraction>1){fraction=1;remainder=0;}current=remainder*start+fraction*stored;}
};
}
