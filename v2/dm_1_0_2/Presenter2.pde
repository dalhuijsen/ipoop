class Presenter2 implements IPresenter { 
  HashMap<String, ArrayList<PImage>> buffers = new HashMap<String, ArrayList<PImage>>();
  int cm = 0;
  Rect selectRect = null;
  PImage img, bfrimg, dspimg;
  void onSetup() {
    if ( img == null ) { 
      img = loadImage("../../../images/legs.jpg");
      size(img.width, img.height, P3D);
    }
    noLoop();
  }

  void onDraw() { 
    //println("onDraw");
    background(0);
    image(img, 0, 0);
    if ( cm < 3 ) { 
      if ( selectRect != null ) { 
        if ( bfrimg == null ) { 
          fill(color(0, 255, 0, 123));
          rect(selectRect.x, selectRect.y, selectRect.w, selectRect.h);
        } else { 
          image(bfrimg, selectRect.x, selectRect.y);
        }
      }
    }
  }
  void onMouseDragged() { 
    loop();
    println("dragged"+cm);

    if ( cm < 3 ) { 
      if ( selectRect == null ) 
        selectRect = new Rect(mouseX, mouseY, 0, 0);

      selectRect.w = mouseX-selectRect.x;
      selectRect.h = mouseY-selectRect.y;
    } else {
      cm = 0;
    }
  }
  void onMouseReleased() { 
    //noLoop();
  }
  void onMouseMoved() { 
    if ( selectRect != null && selectRect.x < mouseX && selectRect.y < mouseY && mouseX < selectRect.x+selectRect.w-1 && mouseY < selectRect.y+selectRect.h-1 ) { 
      float xp =  ( 100 * ( ((float)mouseX-(float)selectRect.x) / (float)selectRect.w ) );
      float yp =  ( 100 * ( ((float)mouseY-(float)selectRect.y) / (float)selectRect.h ) );
      println(selectRect.w);
      bfrimg = mockEffect(xp, yp);  


      println("relevant moved" + (int)xp + " x " + (int)yp);
    }
  }
  void onMousePressed() {
  }
  void onMouseClicked() { 
    println("click");
    if (  selectRect != null && selectRect.w != 0 && selectRect.h != 0 ) { 
      selectRect.normalize();println("click2");
      PImage cp = mockEffect(1, 1);
      image(cp, selectRect.x, selectRect.y);
      if ( bfrimg != null ) { 
        println("click3");
       // img = createImage(img.width, img.height, RGB);
        img.copy(bfrimg, 0, 0, selectRect.w, selectRect.h, selectRect.x, selectRect.y, selectRect.w, selectRect.h);
        redraw();
      }
      bfrimg = null;
      println("encore"+cp.pixels.length);
     // img = createImage(img.width, img.height, RGB);
      background(0);
      image(img,0,0);
      noLoop();
      if ( cm != 3 )  {
        cm = 3;
      } else {
        println("click reset");
        cm = 0;
        selectRect = null;
      }

      //selectRect = null;
    }
  }
  PImage mockEffect(float ix, float iy) { 
    PImage cp = createImage(selectRect.w, selectRect.h, RGB);
    int dx, dy;
    cp.copy(img, dx = selectRect.x, dy = selectRect.y, selectRect.w, selectRect.h, 0, 0, cp.width, cp.height);
    cp = new MockEffect().run(cp, ix, iy);
    return cp;
  }
}

