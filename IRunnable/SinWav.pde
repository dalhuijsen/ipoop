class SinWav implements IRunnableEffect {  
PImage run(PImage img, float xp, float yp) { 
  int ctr = 0;  
  img.loadPixels();
  float ylength = map(xp,0,100,1,100);
  float xlength = map(yp,0,100,1,100);
  PImage result = createImage(img.width,img.height,RGB);
  for ( int y =0, h = img.height; y<h; y++) { 
    for ( int x = 0, w = img.width; x<w;x++, ctr++) {
      int pos = x + y * w;
      color c = img.pixels[pos];
      int epos = (int)(( x + sin(y/ylength)*xlength)+ y * w);
      if ( epos < result.pixels.length )
        result.pixels[epos] = c;
      else
        result.pixels[pos] = c;
    }
  }
  result.updatePixels();
  return result;   
}
}
