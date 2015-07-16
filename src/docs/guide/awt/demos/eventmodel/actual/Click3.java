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

import java.awt.*;
import java.awt.event.*;
import java.applet.*;


/*
 * The final version of the Click applet adds some additional logic
 * to make a pathetic little game out of the parts we've assembled.
 * Note that we've made a significant change to the TargetListener
 * class: it's no longer static.  As an ordinary nested class it has
 * access to the currentTarget field which it writes each time the
 * mouse enters or exits a target.  The MouseListener called shootTarget
 * reads this field when the user clicks the mouse.
 * 
 * This applet runs correctly in HotJava, it requires JDK 1.1.
 */


public class Click3 extends Applet
{
  Color puckColor = new Color(200, 0, 10);
  Box puck = new Box(puckColor);
  ColumnOfBoxes[] targets = new ColumnOfBoxes[8];
  ColumnOfBoxes currentTarget;

  private final class TargetListener 
    extends MouseAdapter implements MouseMotionListener
  {
    private Color newBackground;
    private Color oldBackground;

    TargetListener(Color newBackground) {
      this.newBackground = newBackground;
    }

    public void mouseEntered(MouseEvent e) {
      oldBackground = e.getComponent().getBackground();
      e.getComponent().setBackground(newBackground);
      currentTarget = (ColumnOfBoxes)(e.getComponent());
    }

    public void mouseExited(MouseEvent e) {
      e.getComponent().setBackground(oldBackground);
      currentTarget = null;
    }

    private void redispatch(MouseEvent e) {
      Point origin = e.getComponent().getLocation();
      e.translatePoint(origin.x, origin.y);
      e.getComponent().getParent().dispatchEvent(e);
    }

    public void mouseMoved(MouseEvent e) {  redispatch(e); }
    public void mouseDragged(MouseEvent e) { redispatch(e); }
    public void mouseClicked(MouseEvent e) { redispatch(e); }
  }

  public Click3()
  {
    MouseMotionListener movePuck = new MouseMotionAdapter() {
      public void mouseMoved(MouseEvent e)
      {
	int x = e.getX() - (puck.getSize().width / 2);
	int y = getSize().height - puck.getSize().height;
	puck.setLocation(x, y);
      }
    };

    MouseListener shootTarget = new MouseAdapter() {
      public void mouseClicked(MouseEvent e)
      {
	if (currentTarget != null) {
	  int nBoxes = currentTarget.getComponentCount();
	  if (nBoxes == e.getClickCount()) {
	    currentTarget.removeAll();
	    currentTarget.getToolkit().beep();
	    currentTarget.repaint();
	  }
	}
      }
    };

    for(int i = 0; i < targets.length; i++) {
      int nBoxes = 1 + (int)(Math.random() * 4.0);
      float boxHue = (float)i / (float)targets.length;
      Color boxColor = Color.getHSBColor(boxHue, 0.5f, 0.85f);
      TargetListener tl = new TargetListener(boxColor.brighter());
      targets[i] = new ColumnOfBoxes(boxColor, nBoxes);
      targets[i].addMouseListener(tl);
      targets[i].addMouseMotionListener(tl);
      add(targets[i]);
    }

    add(puck);
    addMouseMotionListener(movePuck);
    addMouseListener(shootTarget);
  }

  public static void main(String[] args)
  {
    WindowListener l = new WindowAdapter()
      {
	public void windowClosing(WindowEvent e) {System.exit(0);}
      };

    Frame f = new Frame("Click");
    f.addWindowListener(l); 
    f.add(new Click3());
    f.setSize(600, 400);
    f.show();
  }

  public String getAppletInfo() 
  { 
    return "Click3 by Hans Mueller. The final version of the Click applet adds some additional logic to make a pathetic little game out of the parts we've assembled.  Note that we've made a significant change to the TargetListener class: it's no longer static.  As an ordinary nested class it has access to the currentTarget field which it writes each time the mouse enters or exits a target.  The MouseListener called shootTarget reads this field when the user clicks the mouse.  This applet requires JDK 1.1.";
  }
  
  public String[][] getParameterInfo() {
    return null;
  }   
  
}
