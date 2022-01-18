import processing.serial.*;

Serial port;

final int DELIMITER = '\n';

// points around the circle
final int numberOfPoints = 75;
// self-documenting
final int numberOfLinesToDrawPerFrame = 1;

//------------------------------------------------------

// WeavingThread class tracks a thread as it is being woven around the nails.
class WeavingThread {
  // thread color (hex value)
  public color c;
  // thread color name (human readable)
  public String name;
  // last nail reached
  public int currentPoint;
};

// for tracking the best place to put the next weaving thread.
class BestResult {
  // nails
  public int bestStart,bestEnd;
  // score
  public float bestValue;
  
  public BestResult( int a, int b, float v) {
    bestStart=a;
    bestEnd=b;
    bestValue=v;
  }
};

// for re-drawing the end result quickly.
class FinishedLine {
  // nails
  public int start,end;
  // hex color
  public color c;
  
  public FinishedLine(int s,int e,color cc) {
    start=s;
    end=e;
    c=cc;
  }
};

//------------------------------------------------------

// finished lines in the weave.
ArrayList<FinishedLine> finishedLines = new ArrayList<FinishedLine>(); 

// threads actively being woven
ArrayList<WeavingThread> threads = new ArrayList<WeavingThread>();

// stop when totalLinesDrawn==totalLinesToDraw
int totalLinesDrawn=0;

// diameter of weave
float diameter;

// can we start weaving yet?!
boolean ready;

// set true to start paused.  click the mouse in the screen to pause/unpause.
boolean paused=false;

// make this true to pause after every frame.
boolean singleStep=false;

PGraphics dest; 

// nail locations
float [] px = new float[numberOfPoints];
float [] py = new float[numberOfPoints];

// distance from nail n to nail n+m
float [] lengths = new float[numberOfPoints];

int chaser = 0;
int runner = 38;
int fives = 0;

// which results to draw on screen.
int showImage;

long startTime;

// if all threads are tested and they're all terrible, move every thread one nail to the CCW and try again.
// if this repeats for all nails?  there are no possible lines that improve the image, stop.
int moveOver = 0;

boolean finished=false;

// center of image for distance tests
float center;
  
// a list of all nail pairs already visited.  I don't want the app to put many
// identical strings on the same two nails, so WeavingThread.done[] tracks 
// which pairs are finished.
public char [] done = new char[numberOfPoints*numberOfPoints];

final color black   = color(  0,   0,   0, 255);
final color white   = color(  255,   255,   255, 255);

final int totalLinesToDraw = 400;


//------------------------------------------------------

// run once on start.
void setup() {
  // make the window.  must be (h*2,h+20)
  size(800,820);
   println(Serial.list());
   println(Serial.list()[3]);
   port = new Serial(this, Serial.list()[3], 9600);
   port.bufferUntil(DELIMITER); 

  ready=false;
  dest = createGraphics(1000,1000);
  setBackgroundColor();
  setupNailPositions();
  setupThreadsToUse();  
  startTime=millis();
  ready=true;
}

void setupThreadsToUse() {
  threads.add(startNewWeavingThread(black,"black"));
}

void setupNailPositions() {
  setupNailPositionsInACircle();
}

void setupNailPositionsInACircle() {
  println("Circle design");
  // find the size of the circle and calculate the points around the edge.
  diameter = 1000;
  float radius = (diameter/2)-1;
  
  for(int i=0; i<numberOfPoints; ++i) {
    float d = PI * 2.0 * i/(float)numberOfPoints;
    px[i] = 1000 /2 + cos(d) * radius;
    py[i] = 1000 /2 + sin(d) * radius;
  }
}

void setBackgroundColor() {
  setBackgroundColorTo(white);
}

void setBackgroundColorTo(color c) {
  dest.beginDraw();
  dest.background(c);
  dest.endDraw();
}

// setup a new WeavingThread and place it on the best pair of nails.
WeavingThread startNewWeavingThread(color c,String name) {
  WeavingThread wt = new WeavingThread();
  wt.c=c;
  wt.name=name;
  return wt;
}

void mouseReleased() {
  paused = paused ? false : true;
}

void draw() {
  if(!ready) return;
  
  // if we aren't done
  if (!finished) {
    if (!paused) {
      dest.loadPixels();
      drawLine(threads.get(0), (chaser % numberOfPoints), (runner % numberOfPoints));
      fives++;
      if (fives > 4){
        fives = 0;
        chaser++;
      }
      chaser++;
      drawLine(threads.get(0), (runner % numberOfPoints), (chaser % numberOfPoints));
      runner++;
      if (singleStep) paused=true;
      if (chaser == runner) finished=true;
    }
  } else {
    // finished!    
    calculationFinished();
  }
  image(dest, 0, 0, width, height);
  
  drawProgressBar();
}

void drawProgressBar() {
  float percent = (float)totalLinesDrawn / (float)totalLinesToDraw;

  strokeWeight(10);  // thick
  stroke(0,0,255,255);
  line(10, 5, (width-10), 5);
  if(paused) {
    stroke(255,0,0,255);
  } else {
    stroke(0,255,0,255);
  }
  line(10, 5, (width-10)*percent, 5);
}

// stop drawing and ask user where (if) to save CSV.
void calculationFinished() {
  if(finished) return;
  finished=true;
  
  long endTime=millis();
  println("Time = "+ (endTime-startTime)+"ms");
  selectOutput("Select a destination CSV file","outputSelected");
}

// write the file if requested
void outputSelected(File output) {
  if(output==null) {
    return;
  }
  // write the file
  PrintWriter writer = createWriter(output.getAbsolutePath());
  writer.println("Color, Start, End");
  for(FinishedLine f : finishedLines ) {
    
    writer.println(getThreadName(f.c)+", "
                  +f.start+", "
                  +f.end+", ");
  }
  writer.close();
}


String getThreadName(color c) {
  for( WeavingThread w : threads ) {
    if(w.c == c) {
      return w.name;
    }
  }
  return "??";
}

// commit the new line to the destination image (our results so far)
// also remember the details for later.
void drawLine(WeavingThread wt,int a,int b) {
  //println(totalLinesDrawn+" : "+wt.name+"\t"+bestStart+"\t"+bestEnd+"\t"+maxValue);
  
  drawToDest(a, b, wt.c);
  totalLinesDrawn++;
  // move to the end of the line.
  wt.currentPoint = b;
}


// draw thread on screen.
void drawToDest(int start, int end, color c) {
  dest.beginDraw();
  dest.stroke(c);
  dest.strokeWeight(1.2);
  dest.line((float)px[start], (float)py[start], (float)px[end], (float)py[end]);
  dest.endDraw();
  
  finishedLines.add(new FinishedLine(start,end,c));

  port.write(Integer.toString(end));
  println(end);
   String incoming = null;
   while (incoming == null) {
       incoming = port.readStringUntil(DELIMITER);
       delay(50);
   }
   println(incoming);
}
