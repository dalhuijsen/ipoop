/*
 * Copyright 2002 Sun Microsystems, Inc. All  Rights Reserved.
 *  
 * Redistribution and use in source and binary forms, with or 
 * without modification, are permitted provided that the following 
 * conditions are met:
 * 
 * -Redistributions of source code must retain the above copyright  
 *  notice, this list of conditions and the following disclaimer.
 * 
 * -Redistribution in binary form must reproduce the above copyright 
 *  notice, this list of conditions and the following disclaimer in 
 *  the documentation and/or other materials provided with the 
 *  distribution.
 *  
 * Neither the name of Sun Microsystems, Inc. or the names of 
 * contributors may be used to endorse or promote products derived 
 * from this software without specific prior written permission.
 * 
 * This software is provided "AS IS," without a warranty of any 
 * kind. ALL EXPRESS OR IMPLIED CONDITIONS, REPRESENTATIONS AND 
 * WARRANTIES, INCLUDING ANY IMPLIED WARRANTY OF MERCHANTABILITY, 
 * FITNESS FOR A PARTICULAR PURPOSE OR NON-INFRINGEMENT, ARE HEREBY 
 * EXCLUDED. SUN AND ITS LICENSORS SHALL NOT BE LIABLE FOR ANY 
 * DAMAGES OR LIABILITIES  SUFFERED BY LICENSEE AS A RESULT OF OR 
 * RELATING TO USE, MODIFICATION OR DISTRIBUTION OF THE SOFTWARE OR 
 * ITS DERIVATIVES. IN NO EVENT WILL SUN OR ITS LICENSORS BE LIABLE 
 * FOR ANY LOST REVENUE, PROFIT OR DATA, OR FOR DIRECT, INDIRECT, 
 * SPECIAL, CONSEQUENTIAL, INCIDENTAL OR PUNITIVE DAMAGES, HOWEVER 
 * CAUSED AND REGARDLESS OF THE THEORY OF LIABILITY, ARISING OUT OF 
 * THE USE OF OR INABILITY TO USE SOFTWARE, EVEN IF SUN HAS BEEN 
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGES.
 *  
 * You acknowledge that Software is not designed, licensed or 
 * intended for use in the design, construction, operation or 
 * maintenance of any nuclear facility. 
 */
 
import com.sun.java.swing.*;
import java.awt.*;
import java.applet.*;

/*
 * Gauge - a class that implements a lightweight component
 * that can be used, for example, as a performance meter.
 *
 * Lightweight components are "transparent", meaning that
 * you can see the background of the container behind it.
 *
 */
public class Gauge extends JComponent {
    
  // the current and total amounts that the gauge reperesents
  int current = 0;
  int total = 100;

  // The preferred size of the gauge
  int Height = 18;   // looks good
  int Width  = 250;  // arbitrary 

  /**
   * Constructs a Gauge
   */
  public Gauge() {
      this(Color.lightGray);
  }

  /**
   * Constructs a that will be drawn uses the
   * specified color.
   *
   * @gaugeColor the color of this Gauge
   */
  public Gauge(Color gaugeColor) {
      setBackground(gaugeColor);
  }

  public void paint(Graphics g) {
      int barWidth = (int) (((float)current/(float)total) * getSize().width);
      g.setColor(getBackground());
      g.fill3DRect(0, 0, barWidth, getSize().height-2, true);
  }

  public void setCurrentAmount(int Amount) {
      current = Amount; 

      // make sure we don't go over total
      if(current > 100)
       current = 100;

      repaint();
  }

  public int getCurrentAmount() {
      return current;
  }

  public int getTotalAmount() {
      return total;
  }

  public Dimension getPreferredSize() {
      return new Dimension(Width, Height);
  }

  public Dimension getMinimumSize() {
      return new Dimension(Width, Height);
  }

}

