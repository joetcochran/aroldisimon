using System;
using System.Net;
using System.Net.Sockets;
using System.Threading;
using Microsoft.SPOT;
using Microsoft.SPOT.Hardware;
using SecretLabs.NETMF.Hardware;
using SecretLabs.NETMF.Hardware.NetduinoPlus;
using Servo_API;
using System.IO.Ports;

namespace Aroldis
{
    public class Program
    {
        static int servo_degree = 90;
        static OutputPort greenled;
        static OutputPort blueled;
        static OutputPort redled;
        static SerialPort serial;
        static Servo servo;

        static OutputPort whitebutton;
        static OutputPort blackbutton;
        static OutputPort redbutton;
        static OutputPort bluebutton;
        static OutputPort greenbutton;
        static OutputPort yellowbutton;
        static int customdelay = 800;

        private static bool AddNewStep = false;
        private static bool AcceptInput = true;
        private static System.Collections.ArrayList gamesteps = new System.Collections.ArrayList();
        private static System.Collections.ArrayList usersteps = new System.Collections.ArrayList();
        enum COLORS
        {
            RED,
            GREEN,
            BLUE,
            WHITE
        }
        enum FLASHSPEEDS
        {
            FAST, SLOW, CUSTOM
        }

        public static void Main()
        {
            // write your code here
            servo = new Servo(Pins.GPIO_PIN_D9);
            servo.Degree = servo_degree;

            greenled = new OutputPort(Pins.GPIO_PIN_D3, true);
            blueled = new OutputPort(Pins.GPIO_PIN_D4, true);
            redled = new OutputPort(Pins.GPIO_PIN_D5, true);

            whitebutton = new OutputPort(Pins.GPIO_PIN_D0, true);
            blackbutton = new OutputPort(Pins.GPIO_PIN_D1, true);
            redbutton = new OutputPort(Pins.GPIO_PIN_D2, true);
            bluebutton = new OutputPort(Pins.GPIO_PIN_D6, true);
            greenbutton = new OutputPort(Pins.GPIO_PIN_D7, true);
            yellowbutton = new OutputPort(Pins.GPIO_PIN_D8, true);

            bool whitestate = false;
            bool whitestate_prev = false;
            bool blackstate = false;
            bool blackstate_prev = false;
            bool redstate = false;
            bool redstate_prev = false;
            bool bluestate = false;
            bool bluestate_prev = false;
            bool greenstate = false;
            bool greenstate_prev = false;
            bool yellowstate = false;
            bool yellowstate_prev = false;

            while (true)
            {
                if (AcceptInput)
                {
                    whitestate = whitebutton.Read();
                    if ((whitestate_prev == true) && (whitestate == false))
                    {
                        WhitePressed();
                        RecordAndCheckUserStep(3);
                    }
                    whitestate_prev = whitestate;


                    blackstate = blackbutton.Read();
                    if ((blackstate_prev == true) && (blackstate == false))
                        BlackPressed();
                    blackstate_prev = blackstate;

                    yellowstate = yellowbutton.Read();
                    if ((yellowstate_prev == true) && (yellowstate == false))
                    {
                        YellowPressed();
                        RecordAndCheckUserStep(4);
                    }
                    yellowstate_prev = yellowstate;

                    redstate = redbutton.Read();
                    if ((redstate_prev == true) && (redstate == false))
                    {
                        TurnOffLED(COLORS.RED);
                        RecordAndCheckUserStep(0);
                    }
                    else if ((redstate_prev == false) && (redstate == true))
                        TurnOnLED(COLORS.RED);
                    redstate_prev = redstate;

                    bluestate = bluebutton.Read();
                    if ((bluestate_prev == true) && (bluestate == false))
                    {
                        TurnOffLED(COLORS.BLUE);
                        RecordAndCheckUserStep(2);
                    }
                    else if ((bluestate_prev == false) && (bluestate == true))
                        TurnOnLED(COLORS.BLUE);
                    bluestate_prev = bluestate;

                    greenstate = greenbutton.Read();
                    if ((greenstate_prev == true) && (greenstate == false))
                    {

                        TurnOffLED(COLORS.GREEN);
                        RecordAndCheckUserStep(1);
                    }
                    else if ((greenstate_prev == false) && (greenstate == true))
                        TurnOnLED(COLORS.GREEN);
                    greenstate_prev = greenstate;
                }
            }

        }

        private static void RecordAndCheckUserStep(int userstep)
        {
            usersteps.Add(userstep);

            //check the command that user just put in
            if (CheckUserSteps())
            {

                //user matched all steps correctly. add a new one now
                if (AddNewStep)
                {
                    //make the replay of gamesteps progressively faster
                    customdelay = (int)((double)(customdelay) * 0.8); 
                    System.Threading.Thread.Sleep(500);
                    AddRandomStep();
                    PlayGameSteps();

                    //clear out the user steps for the next set of input
                    usersteps = new System.Collections.ArrayList();

                    //reinitialize the control variable
                    AddNewStep = false;
                }
            }
            else
            {
                //user messed up. fail out and reset timing
                customdelay = 800;
                ShakeHead();
            }
        }

        private static void ShakeHead()
        {
            servo.Degree = 0;
            System.Threading.Thread.Sleep(450);
            servo.Degree = 180;
            System.Threading.Thread.Sleep(700);
            servo.Degree = 0;
            System.Threading.Thread.Sleep(700);
            servo.Degree = 180;
            System.Threading.Thread.Sleep(700);
            servo.Degree = 90;
            servo_degree = 90;

        }

        
        
        private static bool CheckUserSteps()
        {
            if (gamesteps.Count == 0)
            {
                return false;
            }

            //loop over all user steps entered so far and check them against the gamesteps
            for (int i = 0; i < usersteps.Count; i++)
            {
                if (gamesteps[i].ToString() != usersteps[i].ToString())
                    return false;
            }

            //user matched all game steps successfully. set a variable to create a new gamestep
            if (usersteps.Count == gamesteps.Count)
                AddNewStep = true;

            return true;
        }

        

        private static void YellowPressed()
        {
            if (servo_degree >= 180)
            {
                FlashColor(COLORS.RED, 2, FLASHSPEEDS.FAST);
                return;
            }
            RotateRight();

        }

        private static void WhitePressed()
        {
            if (servo_degree <= 0)
            {
                FlashColor(COLORS.RED, 2, FLASHSPEEDS.FAST);
                return;
            }
            RotateLeft();
        }

        private static void BluePressed()
        {
            FlashColor(COLORS.BLUE, 1, FLASHSPEEDS.SLOW);
        }

        private static void RedPressed()
        {
            FlashColor(COLORS.RED, 1, FLASHSPEEDS.SLOW);
        }


        private static void BlackPressed()
        {
            gamesteps = new System.Collections.ArrayList();
            usersteps = new System.Collections.ArrayList();
            AddRandomStep();
            PlayGameSteps();
            customdelay = 800;
        }


        private static void PlayGameSteps()
        {
            AcceptInput = false;
            servo.Degree = 90;
            servo_degree = 90;
            System.Threading.Thread.Sleep(100);
            TurnOffLED(COLORS.WHITE);
            foreach (int step in gamesteps)
            {
                System.Threading.Thread.Sleep(500);

                if (step == 0)
                    FlashColor(COLORS.RED, 1, FLASHSPEEDS.CUSTOM);

                if (step == 1)
                    FlashColor(COLORS.GREEN, 1, FLASHSPEEDS.CUSTOM);

                if (step == 2)
                    FlashColor(COLORS.BLUE, 1, FLASHSPEEDS.CUSTOM);

                if (step == 3)
                {
                    RotateLeft();
                    int sleeptime = (int)(100 * System.Math.Pow((double)0.9, ((double)gamesteps.Count)));
                    System.Threading.Thread.Sleep(sleeptime);
                }
                if (step == 4)
                {
                    RotateRight();
                    int sleeptime = (int)(100  * System.Math.Pow((double)0.9, ((double)gamesteps.Count)));
                    System.Threading.Thread.Sleep(sleeptime);
                }

            }
            System.Threading.Thread.Sleep(500);
            servo.Degree = 90;
            servo_degree = 90;
            

            AcceptInput = true;
        }


        private static void AddRandomStep()
        {            
            int step;
            bool isvalidstep = false;
            do
            {
                System.Random r = new Random();
                double rand = r.NextDouble() * 5.00;

                step = (int)(rand);
                if ((step >= 0) && (step <= 2)) //all color steps are valid
                    isvalidstep = true;
                if ((step == 3) && (servo_degree > 0)) //don't add a rotateleft step if we're already at the max rotation
                    isvalidstep = true;
                if ((step == 4) && (servo_degree < 180)) //don't add a rotateright step if we're already at the max rotation
                    isvalidstep = true;


            } while (!isvalidstep);
                gamesteps.Add(step);
        }


        private static void RotateLeft()
        {
            servo_degree = servo_degree - 30;
            servo.Degree = servo_degree;

        }
        private static void RotateRight()
        {
            servo_degree = servo_degree + 30;
            servo.Degree = servo_degree;
        }
        
        

        private static void FlashColor(COLORS color, int flashes, FLASHSPEEDS speed)
        {
            int delayms;
            if (speed == FLASHSPEEDS.FAST)
            {
                delayms = 200;
            }
            else if (speed == FLASHSPEEDS.SLOW)
            {
                delayms = 800;
            }
            else
            {
                delayms = customdelay;
            }


            for (int i = 0; i < flashes; i++)
            {
                TurnOnLED(color);
                Thread.Sleep(delayms);
                TurnOffLED(color);
                if (speed == FLASHSPEEDS.CUSTOM)
                    Thread.Sleep(delayms / 2);
                else
                    Thread.Sleep(delayms);
            }
        }

        private static void TurnOnLED(COLORS color)
        {
            switch (color)
            {
                case COLORS.BLUE:
                    blueled.Write(false);
                    break;
                case COLORS.RED:
                    redled.Write(false);
                    break;

                case COLORS.GREEN:
                    greenled.Write(false);
                    break;
                case COLORS.WHITE:
                    blueled.Write(false);
                    greenled.Write(false);
                    redled.Write(false);
                    break;

            }
        }
        private static void TurnOffLED(COLORS color)
        {
            switch (color)
            {
                case COLORS.BLUE:
                    blueled.Write(true);
                    break;
                case COLORS.RED:
                    redled.Write(true);
                    break;

                case COLORS.GREEN:
                    greenled.Write(true);
                    break;
                case COLORS.WHITE:
                    blueled.Write(true);
                    greenled.Write(true);
                    redled.Write(true);
                    break;
            }
        }
    }
}
