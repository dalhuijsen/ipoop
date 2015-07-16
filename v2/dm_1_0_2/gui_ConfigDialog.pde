class ConfigDialog { 
  Rect _bound;
  IConfigurableEffect _effect;
  Rect bound() { 
    return _bound;
  }
  void bound(Rect rect) { 
    _bound = rect;
  } 
  ConfigDialog() {
  }
  ConfigDialog(IConfigurableEffect effect) {
    _effect = effect;
  }
  void onDraw() { 
    if ( controls == null ) { 
      controls = createControlList();
    }
    for(IControl c : controls ) { 
      c.onDraw();
    }
  }
  ArrayList<IControl> controls = null;
  ArrayList<IControl> createControlList() { 
    ArrayList<IControl> result = new ArrayList<IControl>();
    if ( _bound == null )  _bound = new Rect(0, 0, 180, 200);
    color menuFgColor = color(0, 255, 0, 200);
    color menuBgColor = color(0, 0, 0, 123);
    color menuTextColor = color(0, 255, 0, 244);
    HashMap<String, Object> conf = _effect.config();
    //  rect(_bound.x, _bound.y, _bound.w, _bound.h);
    if ( conf != null ) { 
      int rows = conf.size();
      int rowh = _bound.h/rows;
      int rowidx = 0;
      HashMap<String, ConfigDefinition> defs = _effect.defaults();
      for (String k : conf.keySet ()) { 
        Object kv = conf.get(k);
        int cx = _bound.x, 
        cy = _bound.y+(rowidx*rowh), 
        cw = _bound.w, 
        ch = rowh;
        if ( kv != null ) { 
          String cn = kv.getClass().getName();
          //println("]"+cn+"[");
          ConfigDefinition cd;
          if ( ( cd = defs.get(k))  != null ) { 
            if ( cd.type.equals("color") ) {
              ColorControl cc = new ColorControl(cx, cy, cw, ch);
              cc.title = k;
              cc.value = (Integer)kv;
              result.add(cc);
              //cc.onDraw();
            } else {
              println("no control for type"+cd.type);
            }
          } else if ( cn.equals("java.lang.Integer") ) { 
            IntegerControl ic = new IntegerControl(cx, cy, cw, ch);
            ic.title = k;
            ic.value = (Integer)kv;
            result.add(ic);
            //ic.onDraw();
          } else if ( cn == "java.lang.String" ) {
            StringControl sc = new StringControl(cx, cy, cw, ch);
            sc.title = k;
            sc.value = (String)kv;
            result.add(sc);
            //sc.onDraw();
          } else {               
            println("N o H A N D L E R for ]"+cn+"[");
          }
        }
        rowidx++;
      }
    }
    return result;
  }
}

