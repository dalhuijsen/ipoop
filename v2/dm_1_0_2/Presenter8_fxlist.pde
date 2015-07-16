class Presenter8 implements IPresenter { 
  class EffectControl { 
     IEffect effect;
     IControl control;
     EffectControl(IEffect fx, IControl ctrl) { effect = fx; control = ctrl; }
  }
  int w = width, h = height;
  HashMap<Rect, EffectControl> handlers = new HashMap<Rect, EffectControl>();
  ArrayList<IControl> controls = null;
  void onSetup() {
    background(state.bgColor);
  }
  void onMouseClicked() {
  }
  void onDraw() {
    int startx = 0, starty = 0, curx = startx, cury = starty;
    int ts = 20;
    int bw = 100;
    int bh = ts + 4;
    textSize(ts);
    textAlign(LEFT, TOP);
    stroke(state.fgMenuColor);
    fill(state.bgMenuColor);
    if ( controls == null ) { 
      controls = new ArrayList<IControl>();
      for (String s : state.fx.keySet () ) { 

        FxListEffectButton btn = new FxListEffectButton(curx, cury, bw, bh);
        btn.title(s);
        controls.add(btn);
        handlers.put(new Rect(curx, cury, bw, bh), new EffectControl(state.fx.get(s) ,btn) );
        curx += bw;
        if ( curx+bw > w ) { 
          curx = startx;
          cury += bh;
        }
      }
    }
    if ( controls != null )
      for (IControl c : controls) { 
        c.onDraw();
      }
  }
  void onMousePressed() {
  }
  void onMouseDragged() {
  }
  void onMouseReleased() {
    for (Rect c : handlers.keySet ()) {
      if ( mouseX >= c.x  && mouseY >= c.y && mouseX < c.x+c.w && mouseY < c.y+c.h ) { 
        state.currentEffect = handlers.get(c).effect;
        println(c.x, c.w, mouseX, width);
        redraw();
      }
    }
  }
  void onMouseMoved() {
  }
}

class FxListEffectButton extends BaseControl {
  String title = "";
  int x, y, w, h;
  FxListEffectButton(int xx, int yy, int ww, int hh) { 
    super(xx, yy, ww, hh);
    x=xx;
    y=yy;
    w=ww;
    h=hh;
  }
  void title(String fx) { 
    title = fx;
  }
  void onDraw() { 
    textAlign(LEFT, TOP);
    stroke(state.fgMenuColor);
    fill(state.bgMenuColor);
    rect(x, y, w, h);
    fill(state.fgTextColor);
    text(title, x, y, w, h);
  }
}

