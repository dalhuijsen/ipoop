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

package actual;

import java.applet.*;
import java.util.*;
import java.awt.*;

/**
 * This applet creates a window that has a pretty background 
 * picture, and adds several lightweight Gauge objects.
 *
 * Notice how parts of the lightweight Gauges are "transparent" 
 * and you can see the image behind them! Cool!
 */
public class ExampleApplet extends Applet {

  public void init() {
    
    setLayout(new BorderLayout());
    
    // create a double buffer panel and add it to the 
    // center of the frame
    DoubleBufferPanel dbp = new DoubleBufferPanel();
    dbp.setLayout(new BorderLayout());
    add("Center", dbp);
    
    // create a pretty panel and add it to the
    // double buffer panel
    
    //needed because ExampleApplet is running under Switcher
    Applet parentApplet;
    
    //Get the parent Applet object. 
    try {
      parentApplet = (Applet)getParent();
      PrettyPanel pp = new PrettyPanel(parentApplet);
      pp.setLayout(new FlowLayout());
      dbp.add("Center", pp);
      
      // *** Create Gauges
      Gauge gauge1  = new Gauge(Color.green.darker());
      pp.add(gauge1);
      
      Gauge gauge2  = new Gauge(Color.red.darker());
      pp.add(gauge2);
      
      Gauge gauge3 = new Gauge(Color.cyan);
      pp.add(gauge3);
      
      Gauge gauge4 = new Gauge();
      pp.add(gauge4);
      
      Gauge gauge5 = new Gauge(Color.blue.darker());
      pp.add(gauge5);
      
      Gauge gauge6 = new Gauge(Color.pink.darker());
      pp.add(gauge6);
      
      Gauge gauge7 = new Gauge(Color.yellow);
      pp.add(gauge7);
      
      // *** Create threads to drive the gauges
      GaugeThread gaugeThread1 = new GaugeThread(gauge1);
      gaugeThread1.start();
      
      GaugeThread gaugeThread2 = new GaugeThread(gauge2);
      gaugeThread2.start();
      
      GaugeThread gaugeThread3 = new GaugeThread(gauge3);
      gaugeThread3.start();
      
      GaugeThread gaugeThread4 = new GaugeThread(gauge4);
      gaugeThread4.start();
      
      GaugeThread gaugeThread5 = new GaugeThread(gauge5);
      gaugeThread5.start();
      
      GaugeThread gaugeThread6 = new GaugeThread(gauge6);
      gaugeThread6.start();
      
      GaugeThread gaugeThread7 = new GaugeThread(gauge7);
      gaugeThread7.start();
      
    } catch (ClassCastException e) {
      System.err.println("Parent isn't an Applet!");
      throw(e);
    }
  }
}


class PrettyPanel extends Container {
  Image background;

  PrettyPanel(Applet applet) {
      background = applet.getImage(applet.getCodeBase(),
				   "images/mandrill.jpg");
  }

  /**
   * override update to *not* erase the background before painting
   */
  public void update(Graphics g) {
      paint(g);
  }
 
  /**
   * paint the background picture, then call super.paint which
   * will paint all contained components 
   *
   * Note: You MUST call super.paint(g) or the lightweight
   * component(s) won't get painted.
   */
  public void paint(Graphics g) {
      g.drawImage(background, 0, 0, getSize().width, getSize().height,
                  getBackground(), this);
       super.paint(g); 
  }
}

class GaugeThread extends Thread {

  Gauge gauge;
  static int seed = 1;

  GaugeThread(Gauge gauge) {
      super("Gauge thread");
      this.gauge = gauge;
  }
 
  public void run () {
      Random rand = new Random(seed++);
      int i = gauge.getTotalAmount()/2;
      while(true) {
          float r = rand.nextFloat();
          if(r > .5) {
	      if(i < gauge.getTotalAmount())
                i+=2;
          } else {
	      if(i > 0)
		i-=2;
          }
	  
          gauge.setCurrentAmount(i);
          try {
              sleep(100);
          } catch (java.lang.InterruptedException e) {
          }
      }
  }
}
