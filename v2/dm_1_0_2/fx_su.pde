PImage decodr(PImage img, float xp, float yp) { 
  int gh = 20; //grid height
  int gw = 30; //grid width;
  PImage result = createImage(img.width,img.height,RGB);
  for (int y = img.height-1; y>-1; y-=gh ) { 
    for ( int x = img.width-1; x>-1; x-=gw ) { 
      if ( random(1) > 0.4 ) {
        int tx = x+(int)(random(1) * (x-(img.width-gw)/5));
        int ty = y; //(int)(random(1) * (img.width-gw));
        result.copy(img,tx,ty,gw,gh,x,y,gw,gh);
        result.copy(img,x,y,gw,gh,tx,ty,gw,gh);
      } else { 
        result.copy(img,x,y,gw,gh,x,y,gw,gh);        
      }
    }
  }
  return result;
}
