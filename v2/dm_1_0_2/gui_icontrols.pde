
class ColorControl extends BaseControl { 
  ColorControl(int xx, int yy, int ww, int hh) { 
    super(xx, yy, ww, hh);
  }
}
class StringControl extends BaseControl { 
  StringControl(int xx, int yy, int ww, int hh) { 

    super(xx, yy, ww, hh);
    println("last");
  }
}

class BaseControl implements IControl { 
  int cx, cy, cw, ch;
  String title;
  Object value;
  Boolean hasFocus = false;

  void onKeyPressed() { 
    if (!hasFocus) return;
  }
  void onMouseDragged() {
    if (!hasFocus) return;
  }
  void onMouseClicked() { 
    if (!hasFocus) return;
  }
  void onMouseReleased() { 
    if (!hasFocus) return;
  }
  void onMouseMove() { 
    if (!hasFocus) return;
  }
  void reset() {
  }
  void value(Object val) { 
    this.value =  val;
  }
  Object value() { return this.value; }
  ConfigDefinition config;
  BaseControl() {}
  BaseControl(int xx, int yy, int ww, int hh) { 
    cx = xx;
    cy = yy;
    ch = hh;
    cw = ww;
    println(ch);
  }
  void onDraw() { 
    //bg
    println(ch);
    fill(state.bgMenuColor);
    stroke(state.fgMenuColor);
    rect(cx, cy, cw, ch);
    //title
    textAlign(LEFT, TOP);
    textSize(ch*0.3);
    fill(state.fgTextColor);
    text(title, cx, cy);
    //value
    textAlign(RIGHT, BOTTOM);
    textSize(ch*0.2);
    text(""+value, cx, cy, cw, ch);
  }
}

class IntegerControl extends BaseControl { 
  IntegerControl(int xx, int yy, int ww, int hh) { 
    super(xx, yy, ww, hh);
    cx = xx;
    cy = yy;
    ch = hh;
    cw = ww;
  }
  void onDraw() { 
    //bg
    println(ch);
    fill(state.bgMenuColor);
    stroke(hasFocus?state.hlMenuColor:state.fgMenuColor);
    rect(cx, cy, cw, ch);
    //title
    textAlign(LEFT, TOP);
    textSize(ch*0.3);
    fill(state.fgTextColor);
    text(title, cx, cy);
    //value
    textAlign(RIGHT, BOTTOM);
    textSize(ch*0.2);
    text(""+value, cx, cy, cw, ch);
  }
}


