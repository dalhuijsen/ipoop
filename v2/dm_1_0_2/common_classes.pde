
class Rect { 
  Rect(int mx, int my, int mw, int mh) { 
    this.x = mx;
    this.y = my;
    this.h = mh;
    this.w = mw;
   // this.normalize();
  }
  void normalize() { 
    if ( this.w < 0 ) 
      this.x = this.x-(this.w=abs(this.w));
    if ( this.h < 0 ) 
      this.y = this.y-(this.h=abs(this.h));
  }
  int x = 0; 
  int y = 0; 
  int h = 0; 
  int w = 0;
}



class ConfigDefinition { 
  String type;
  String key;
  Object minValue;
  Object maxValue;
  Class dataType; 
  ConfigDefinition(String t, String k, Object mi, Object ma, Class dt) { 
    type = t; this.key = k; minValue = mi; maxValue = ma; dataType = dt;
  }
}



class ImageMgr {
}

class FxMgr { 
  PImage img;
  float xp = 0.0, yp = 100.0;
  Rect rect;
  IConfigurableEffect effect;
  PImage result() { 
    if ( img == null )  return null;
    if ( rect != null && effect != null ) { 
      return effect.run(img, xp, yp);
    }
    return null;
  }
}


