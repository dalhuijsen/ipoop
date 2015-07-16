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
import java.lang.*;
import java.util.*;
import java.awt.*;

/**
 * Spinner - a class that creates a lightweight component that
 * shows a spinning wheel.
 *
 * Lightweight components can have "transparent" areas, meaning that
 * you can see the background of the container behind these areas.
 *
 */
public class Spinner extends Component {

  float percentDone = 0;
  int totalTicks    = 60;
  int currentTick   = 0;
  
  SpinnerThread spinnerThread;
  
  /**
   * Constructs a Spinner
   */
  public Spinner() {
      setForeground(Color.gray);
      setForeground(Color.lightGray);
  }
  
  /**
   * paints the Spinner
   */
  public void paint(Graphics g) {
      int start_angle = 90;
      int done_angle = (int) (percentDone * 360);
      
      g.setColor(getBackground());
      g.fillArc(3, 3, getSize().width-8, getSize().height-8, 0, 360);
      
      g.setColor(getForeground());
      g.fillArc(3, 3, getSize().width-8, getSize().height-8, start_angle, done_angle);

      g.setColor(Color.black);
      g.drawArc(3, 3, getSize().width-8, getSize().height-8, 0, 360);
  }

  public void setCurrentTick(int tick) {
      currentTick = tick;

      if(currentTick > totalTicks) {
	  percentDone = 1;
      } else if(currentTick == 0) {
	  percentDone = 0;
      } else {
	  percentDone = (float) currentTick / (float) totalTicks;
      }
      
      // Repaint might flicker a bit. To avoid this, you can use
      // double buffering (see the Gauge example).
      repaint();
  }

  public void startSpinning() {
      spinnerThread = new SpinnerThread(this);
      spinnerThread.start();
  }

  public void stopSpinning() {
      spinnerThread.stop();
      spinnerThread = null;
  }

  public void setTotalTicks(int tick) {
      totalTicks = tick;
  }

  public int getTotalTicks() {
      return totalTicks;
  }

  public int getCurrentTick() {
      return currentTick;
  }


}



/**
 * SpinnerThread: spins the wheel
 */
class SpinnerThread extends Thread {

  Spinner spinner;

  SpinnerThread(Spinner spinner) {
      super("Spinner Thread");
      this.spinner = spinner;
  }

  public void run () {
      int i = spinner.getCurrentTick();
      while(true) {
	  try {
	      while (i-- > 0) {
		  spinner.setCurrentTick(i);
		  sleep(100);
	      }
	  } catch (java.lang.InterruptedException e) {
	      // don't care if we are interrupted
	  }
	  i = spinner.getTotalTicks();
      }
  }
}
 
