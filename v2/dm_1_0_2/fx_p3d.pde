class PointsExtrude implements IEffect { 
  ArrayList points;
  PImage img2;
  float phi = 1.618;
  float sizrat = 1.1;
  Boolean saverequested = true;
  int colcount = 33;
  float bright = 1.124;
  float scalrat = 0.7;
  float xoffset;
  float yoffset;

  PointsExtrude() {
    //noLoop();
  }




  int prevx, prevy, prevz;

  PImage run(PImage img1, float xp, float yp) {
    points = new ArrayList();
    sizrat = map(xp, 0, 100, 0.6, 2.0);
    yoffset = (img1.height * sizrat * phi * 1.1);
    xoffset  = (img1.width * sizrat * 1.234);
    PGraphics pg = createGraphics(int(img1.width * sizrat * 2), int(img1.height * sizrat * 2 ), P3D);
    pg.beginDraw();
    //  pg.size(int(img1.width * sizrat * 2), int(img1.height * sizrat * 2 ), P3D);
    pg.smooth();
    background(0);
    int ix = 0, iy = 0, iw = img1.width, ih = img1.height;
    for (iy = 0; iy<ih; iy++) {
      prevx = 0; 
      prevy = 0; 
      prevz = 0;
      for (ix = 0; ix<iw; ix++) { 
        int loc = ix+(iy*iw);
        if ( loc % int( iw / colcount ) == 0 ) {
          color c = img1.pixels[loc];
          c = color(red(c)*bright, green(c)*bright, blue(c)*bright, 220-(brightness(c)/3));
          drawPixel(pg, int(ix*sizrat), int(iy*sizrat), c);
        }
      }
    }
    pg.endDraw();
    return (PImage)pg;
  }



  void mouseClicked() { 
    saverequested = true;
  }

  void drawPixel(PGraphics pg, int x, int y, color c) {
    pg.pushMatrix();
    pg.rotateX(radians(40));
    pg.scale(scalrat*0.816);
    pg.stroke(c);
    int z = -330 + int( red(c)+green(c)+blue(c)/765 * 255);
    //int z = int(-300+(red(c)*phi));
    x = int(x + xoffset);
    y = int(y + yoffset);
    if ( prevx != 0 ) {
      pg.line(x, y, z, prevx, prevy, prevz);
      pg.fill(red(c), green(c), blue(c), 150);
      pg.noStroke();
      pg.translate(0, 0, -240);
      pg.rect(x, y, sizrat*5, sizrat*5);
    }
    prevx = x;
    prevy = y;
    prevz = z;
    pg.popMatrix();
  }
}
