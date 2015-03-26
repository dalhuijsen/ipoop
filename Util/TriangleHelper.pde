class TriangleHelper { 
	Boolean isInTriangle(PVector pt, PVector tp1, PVector tp2, PVector tp3) {
	  // from stack: http://stackoverflow.com/questions/14669614/finding-whether-a-point-is-within-a-triangle
	  float x = pt.x, y = pt.y;
	  float x1 = tp1.x, y1 = tp1.y;
	  float x2 = tp2.x, y2 = tp2.y;
	  float x3 = tp3.x, y3 = tp3.y;
	
	  // no need to divide by 2.0 here, since it is not necessary in the equation
	  double ABC = Math.abs (x1 * (y2 - y3) + x2 * (y3 - y1) + x3 * (y1 - y2));
	  double ABP = Math.abs (x1 * (y2 - y) + x2 * (y - y1) + x * (y1 - y2));
	  double APC = Math.abs (x1 * (y - y3) + x * (y3 - y1) + x3 * (y1 - y));
	  double PBC = Math.abs (x * (y2 - y3) + x2 * (y3 - y) + x3 * (y - y2));
	
	  boolean isInTriangle = ABP + APC + PBC == ABC;
	
	  return isInTriangle;
	}
}
	
