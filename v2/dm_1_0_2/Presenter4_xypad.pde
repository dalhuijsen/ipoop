class Presenter4 implements IPresenter { 
  void onSetup() {
  } 
  void onMouseClicked() {
  } 
  void onDraw() { 
    FxDialogViewData fxdvd = drawFxDialog(state.currentEffect);
  } 
  void onMousePressed() {
  } 
  void onMouseDragged() {
  } 
  void onMouseReleased() {
  } 
  void onMouseMoved() {
  } 





  FxDialogViewData drawFxDialog(IEffect effect) { 
    int rx=width-210, ry = 10;
    int fgcolor = state.fgMenuColor;
    int bgColor = state.bgMenuColor;
    int hlbgcolor = state.hlBgMenuColor;
    FxDialogViewData vd = state.fxData;//new FxDialogViewData(effect); 
    state.fxData = vd;

    //border + bg
    textSize(10);
    
    stroke(fgcolor);
    //noFill();
    fill(bgColor);
    rect(rx, ry, 202, 300);

    //title
    fill(state.fgColor);
    text(effect.getClass().getName(), rx+4, ry+4);
    int curx = rx, cury = ry;

    //boolindicatorline
    cury += 5;
    curx += 2;
    if ( vd.autoRect ) { 
      fill(hlbgcolor);
      text("ar", curx+3, cury+10);
    } else noFill();
    rect(curx, cury, 48, 15);
    noFill();
    curx += 50;
    rect(curx, cury, 48, 15);
    curx += 50;
    rect(curx, cury, 48, 15);
    curx += 50;
    rect(curx, cury, 48, 15);

    //status
    fill(fgcolor);
    stroke(fgcolor);
    curx = rx + 2;
    cury += 30;
    text(" "+mouseX+"x"+mouseY, curx, cury);
    cury += 20;
    if ( vd.targetRect != null ) { 

      String area = String.format("targetting:\n %s.%s.%s.%s\n", vd.targetRect.x, vd.targetRect.y, vd.targetRect.w, vd.targetRect.h);
      text(area, curx, cury);
    }
    cury+=20;

    //xypad

    noFill();
    stroke(fgcolor);
    int padw = 180, padh = 180;
    curx+=10;
    rect(curx, cury, padw, padh);
    float[] markers = new float[] { 
      0.25, 0.5, 0.75
    };
    for ( int i = 0, l = markers.length; i<l; i++ ) { 
      int xxx = curx + (int)((float)padw*markers[i]);
      line(xxx, cury, xxx, cury+(markers[i]!=0&&markers[i]%2==0?10:4));
      line(xxx, -2+cury+(0.5*padw), xxx, -2+cury+(markers[i]!=0&&markers[i]%2==0?10:4)+(0.5*padw));
      int yyy = cury + (int)((float)padh*markers[i]);
      line(curx, yyy, curx+4, yyy);// cury+(markers[i]!=0&&markers[i]%2==0?10:4));
      line(-2+curx+(padw/2), yyy, curx+4+(padw/2), yyy);// cury+(markers[i]!=0&&markers[i]%2==0?10:4));
    }

    //xypad current
    fill(fgcolor);
    int nposx = curx, nposy = cury;
    int nposyh = nposy + padh, nposxw = nposx + padw;
    int dotwidth = 10;
    int dotx = (int)map(vd.xp, 0, 100, nposx, nposxw);
    int doty = (int)map(vd.yp, 0, 100, nposy, nposyh);
    ellipse(dotx, doty, dotwidth, dotwidth);


    curx-=10;
    cury+= padh+10;


    //buttons
    stroke(fgcolor);
    noFill();
    int prevx = curx;
    curx+=10;
    int btnh = 26;
    curx=(int)(curx+padw-((padw-20)*0.33));
    rect(curx, cury, (int)((float)(padw-20)*0.33), btnh);
    text("Go", curx+18, cury+17);
    curx=prevx;
    cury+=btnh;
    return vd;
  }
}

class FxDialogViewData { 
  IEffect effect;
  float xp;
  float yp;
  Rect targetRect;
  Boolean autoRect;
  PImage img;
  FxDialogViewData() { 
    xp = 100.0; 
    yp=100.0; 
    targetRect = null;//new Rect(10,20,49,12); 
    autoRect = true;
    img = loadImage("../../../images/girl7.jpg");
  }
  FxDialogViewData(IEffect f) { 
    this(); 
    this.effect = f;
  }
}

