#pragma region VEXcode Generated Robot Configuration
// Make sure all required headers are included.
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <math.h>
#include <string.h>


#include "vex.h"

using namespace vex;

// Brain should be defined by default
brain Brain;


// START IQ MACROS
#define waitUntil(condition)                                                   \
  do {                                                                         \
    wait(5, msec);                                                             \
  } while (!(condition))

#define repeat(iterations)                                                     \
  for (int iterator = 0; iterator < iterations; iterator++)
// END IQ MACROS

// Robot configuration code.
inertial BrainInertial = inertial();
motor xAxisMotor = motor(PORT9, false);
motor yAxisMotor = motor(PORT5, true);
motor zAxisMotor = motor(PORT4, false);
motor_group driveMotors(xAxisMotor, yAxisMotor, zAxisMotor);
touchled moveTouchLED = touchled(PORT10);
bumper zAxisBumper = bumper(PORT1); 

// generating and setting random seed
void initializeRandomSeed(){
  wait(100,msec);
  double xAxis = BrainInertial.acceleration(xaxis) * 1000;
  double yAxis = BrainInertial.acceleration(yaxis) * 1000;
  double zAxis = BrainInertial.acceleration(zaxis) * 1000;
  // Combine these values into a single integer
  int seed = int(
    xAxis + yAxis + zAxis
  );
  // Set the seed
  srand(seed); 
}

// Converts a color to a string
const char* convertColorToString(color col) {
  if (col == colorType::red) return "red";
  else if (col == colorType::green) return "green";
  else if (col == colorType::blue) return "blue";
  else if (col == colorType::white) return "white";
  else if (col == colorType::yellow) return "yellow";
  else if (col == colorType::orange) return "orange";
  else if (col == colorType::purple) return "purple";
  else if (col == colorType::cyan) return "cyan";
  else if (col == colorType::black) return "black";
  else if (col == colorType::transparent) return "transparent";
  else if (col == colorType::red_violet) return "red_violet";
  else if (col == colorType::violet) return "violet";
  else if (col == colorType::blue_violet) return "blue_violet";
  else if (col == colorType::blue_green) return "blue_green";
  else if (col == colorType::yellow_green) return "yellow_green";
  else if (col == colorType::yellow_orange) return "yellow_orange";
  else if (col == colorType::red_orange) return "red_orange";
  else if (col == colorType::none) return "none";
  else return "unknown";
}


// Convert colorType to string
const char* convertColorToString(colorType col) {
  if (col == colorType::red) return "red";
  else if (col == colorType::green) return "green";
  else if (col == colorType::blue) return "blue";
  else if (col == colorType::white) return "white";
  else if (col == colorType::yellow) return "yellow";
  else if (col == colorType::orange) return "orange";
  else if (col == colorType::purple) return "purple";
  else if (col == colorType::cyan) return "cyan";
  else if (col == colorType::black) return "black";
  else if (col == colorType::transparent) return "transparent";
  else if (col == colorType::red_violet) return "red_violet";
  else if (col == colorType::violet) return "violet";
  else if (col == colorType::blue_violet) return "blue_violet";
  else if (col == colorType::blue_green) return "blue_green";
  else if (col == colorType::yellow_green) return "yellow_green";
  else if (col == colorType::yellow_orange) return "yellow_orange";
  else if (col == colorType::red_orange) return "red_orange";
  else if (col == colorType::none) return "none";
  else return "unknown";
}


void vexcodeInit() {

  // Initializing random seed.
  initializeRandomSeed(); 
}

#pragma endregion VEXcode Generated Robot Configuration

//----------------------------------------------------------------------------
//                                                                            
//    Module:       main.cpp                                                  
//    Author:       {Kevin and Nikita}                                                  
//    Created:      {2026-03-31}                                                    
//    Description:  First iteration of final integration code                                                
//                                                                            
//----------------------------------------------------------------------------

// Include the IQ Library
#include "iq_cpp.h"
#include <algorithm>
#include <climits>
#include <vector>

/*
  ---------- START OF CHESSBOT CODE ---------
*/

#include <algorithm>
#include <climits>
#include <vector>


const short MAX_DEPTH = 5;
const short NM_R = 3,
            NM_DEPTH_INC = NM_R + 1,
            MAX_NM_DEPTH = MAX_DEPTH - NM_DEPTH_INC;
const short MAX_NUM_AXB = 26,
            MAX_2NUM_AXB = MAX_NUM_AXB * 2,
            MAX_3NUM_AXB = MAX_NUM_AXB * 3;

enum Player: short {
    BLACK = 0, WHITE = 1,
    MAX_PLAYER = 2,
    MINER = BLACK, MAXER = WHITE
};
Player operator!(Player player)
{
    return Player(player^1);
}
enum Shape: short {
    PAWN = 0, KNIGHT = 1, BISHOP = 2, ROOK = 3, QUEEN = 4, KING = 5,
    MAX_SHAPE = 6
};
/**
 * SHAPE_PHASE
 * └── 0...5: see enum Shape
 *     └── how much each shape contributes to the game's phase
 * 
 * values from Rofchade: http://www.talkchess.com/forum3/viewtopic.php?f=2&t=68311&start=19
 */
const short SHAPE_PHASE[MAX_SHAPE] = { 0, 0, 1, 1, 2, 4 };

const short PLAY_WIDTH = 8, WIDTH = PLAY_WIDTH + 2, AREA = PLAY_WIDTH*WIDTH, MAX_NUM_PIECE = 16;

/**
 * ZTABLE (Zobrist Table)
 * └── 0,1: see enum Player
 *     └── 0...5: see enum Shape
 *         └── 0...80: index on main board
 *             └── random 32-bit integer
 */
unsigned long ZTABLE[MAX_PLAYER][MAX_SHAPE][AREA] = {{{3894649422UL,2055130073UL,2315086854UL,2925816488UL,3443325253UL,1644475139UL,428639621UL,1241310737UL,0,0,3521718650UL,338531392UL,4000707947UL,1195020567UL,3819198723UL,3169322913UL,3847118669UL,1232956497UL,0,0,1635946854UL,2806007627UL,51974824UL,2648956666UL,3529956337UL,255377083UL,3963375461UL,130352509UL,0,0,3150840776UL,4247439272UL,4291774323UL,1527820283UL,3990997058UL,3794469303UL,3658693772UL,2945036532UL,0,0,634501308UL,2674021506UL,2080204930UL,773984448UL,747616678UL,1120615299UL,1087245739UL,1682584848UL,0,0,908267250UL,3804982595UL,269660995UL,1953262751UL,913394936UL,3978252326UL,2344351806UL,3917943028UL,0,0,2318072374UL,151212873UL,1841407111UL,1747673931UL,3238426959UL,2111278059UL,1653351979UL,2234495538UL,0,0,2975050828UL,1326882075UL,2795157364UL,1272231666UL,3694468563UL,2482977932UL,2735217067UL,4278678924UL,0,0,},{1521564770UL,3513614261UL,3298103290UL,3540627017UL,3110566535UL,1147462423UL,2430354673UL,3331637716UL,0,0,492704104UL,2918414893UL,3661142285UL,516209971UL,919217614UL,4195207018UL,2525666627UL,1766894649UL,0,0,3711071932UL,114665246UL,527378477UL,1549954146UL,3722705969UL,354522318UL,3277771084UL,2985763406UL,0,0,1207397304UL,3008355592UL,1880176990UL,372058697UL,219934879UL,2485692754UL,63292228UL,3545388566UL,0,0,2014773235UL,1808709800UL,2202378369UL,912531709UL,405454554UL,1895311173UL,2937446104UL,3354733730UL,0,0,2844637637UL,2443024826UL,2620877852UL,3985454274UL,1902832117UL,4010160037UL,3996871766UL,1379030616UL,0,0,1638902211UL,4256105200UL,2518479787UL,2670269797UL,1337929797UL,93374617UL,3733984133UL,1157652475UL,0,0,1352511842UL,640657729UL,1279399507UL,1259848222UL,3926725936UL,3732944965UL,1639961877UL,1132798959UL,0,0,},{2256725985UL,177227401UL,4272920933UL,4121242667UL,1501428571UL,2451339921UL,3325427478UL,1146200070UL,0,0,2648309937UL,2001189030UL,1405975435UL,3380612287UL,1637865376UL,230553258UL,1302777769UL,4024662870UL,0,0,4230330791UL,1528789214UL,1514886146UL,3804354619UL,2010820728UL,1044918632UL,1197466133UL,1113899538UL,0,0,2930662045UL,3662081813UL,718547962UL,3490371767UL,3292705543UL,3955039356UL,921957694UL,953649493UL,0,0,1329338440UL,3900324675UL,2536768015UL,1555583817UL,451452917UL,127218398UL,576177222UL,1880422151UL,0,0,244401717UL,671787366UL,3492253308UL,368659760UL,1078386460UL,3093754025UL,4110361439UL,1906511655UL,0,0,2833933883UL,3218294940UL,3685765930UL,2323143568UL,2081102228UL,2061776940UL,2603225807UL,21503803UL,0,0,985927484UL,2550174745UL,3499728549UL,3575081073UL,2184560606UL,2056392472UL,160094910UL,4021001532UL,0,0,},{1418467796UL,3031888395UL,2940024969UL,1274375771UL,3714656746UL,3369575005UL,1155753093UL,3124591965UL,0,0,2874041345UL,1156175542UL,3143991359UL,2180624543UL,822328367UL,2884134865UL,2624826522UL,1343164715UL,0,0,3131646677UL,500460138UL,753490815UL,2461265462UL,727917050UL,769383092UL,2669770100UL,2807463721UL,0,0,1786960107UL,1185541324UL,1349333799UL,18828329UL,3658427409UL,3119232802UL,1337622310UL,1699077521UL,0,0,3848088352UL,1082213105UL,2264058514UL,69522397UL,675222817UL,824031398UL,3369281837UL,865979921UL,0,0,2948225495UL,2483079103UL,999057688UL,2282534629UL,2284986807UL,4108522589UL,1526428819UL,2720677891UL,0,0,1430016647UL,1077560506UL,1054565331UL,2113340974UL,2454724717UL,4030131683UL,1310028811UL,2685267482UL,0,0,4249433346UL,4250623249UL,773802396UL,3999739159UL,2462695345UL,2921890098UL,1579313944UL,3558199783UL,0,0,},{2476809330UL,3776970700UL,206475819UL,1056656090UL,1284591681UL,2173310821UL,3332786731UL,3931382761UL,0,0,1841712627UL,3277170740UL,287960699UL,3975216377UL,3236587542UL,2288372545UL,314380138UL,2543450752UL,0,0,4137756984UL,3287859360UL,4013179787UL,2401966111UL,1307534783UL,2277649270UL,3273410292UL,1554352593UL,0,0,752328125UL,1113247840UL,2291815293UL,1361915428UL,129121731UL,752829835UL,2116300349UL,272677614UL,0,0,694303056UL,3238811718UL,2907279500UL,855216421UL,2236043539UL,1262426900UL,1385910310UL,923663140UL,0,0,260937306UL,2100886500UL,172845548UL,4203362267UL,2948505400UL,3679142703UL,3952371471UL,1545760144UL,0,0,2135996522UL,3460618906UL,452694141UL,3752010969UL,3970580887UL,147772553UL,3505629893UL,3294266524UL,0,0,3925860520UL,84839281UL,496606983UL,885233232UL,954141665UL,579197696UL,951499980UL,3492687387UL,0,0,},{2838568648UL,1445171959UL,1985075810UL,3302596814UL,3153668832UL,1460862628UL,4040601741UL,2090180045UL,0,0,3582366739UL,1977575112UL,2389983823UL,1938280467UL,1032279727UL,3307633508UL,2263867192UL,1351417440UL,0,0,735033104UL,1283988206UL,3920143628UL,2152722613UL,1409924134UL,1305841952UL,117675277UL,2445950950UL,0,0,1762642519UL,2650100217UL,983492593UL,1295528417UL,2908217087UL,3661312431UL,1908484425UL,3346323896UL,0,0,510052592UL,792020859UL,816198582UL,3438897899UL,3364671951UL,2441615004UL,1269023529UL,1918555201UL,0,0,725219701UL,1101589023UL,3103011532UL,3421118480UL,2968286281UL,3190058684UL,961339266UL,2804049969UL,0,0,3696464402UL,882433135UL,1138658372UL,3745533706UL,2759456014UL,3456654661UL,4113554816UL,121967888UL,0,0,2990727247UL,2046793216UL,3099716138UL,2732305613UL,4192938828UL,864733635UL,1848761279UL,2485815758UL,0,0,},},{{369942314UL,995096271UL,806422857UL,997780052UL,110664543UL,2421838905UL,2833563792UL,2164978067UL,0,0,912351387UL,2551325641UL,1720557313UL,2618975619UL,642658171UL,274550223UL,2223091949UL,2975823740UL,0,0,3126316564UL,2865480792UL,3918902488UL,187307870UL,2869383202UL,2807332693UL,3807942731UL,1372156397UL,0,0,3823699024UL,613848374UL,3751330608UL,117240137UL,503693454UL,3510701491UL,840180395UL,1783364723UL,0,0,2873735983UL,307021221UL,1050445823UL,206942635UL,3377731716UL,478693148UL,3729122738UL,3209821563UL,0,0,4228040021UL,390710301UL,1226592432UL,3382700052UL,4118301868UL,4210279481UL,3449099733UL,782586734UL,0,0,3253423955UL,1981690924UL,2909415981UL,2598968175UL,4023085442UL,406333570UL,2304029257UL,2623846390UL,0,0,4235421988UL,209780536UL,1048076612UL,2348951243UL,2354275018UL,3502915411UL,2204261358UL,1122978489UL,0,0,},{527866024UL,1373559398UL,1888063321UL,4223104934UL,2313684326UL,1121519910UL,2708829902UL,252116302UL,0,0,1380116683UL,2119311538UL,3858211703UL,4269007187UL,854095813UL,4104589602UL,45256424UL,685494833UL,0,0,2101744387UL,3967399982UL,119532904UL,2356876957UL,571044022UL,2595211527UL,2658521437UL,4186335919UL,0,0,3041084970UL,1272643902UL,2350711699UL,2318975549UL,2765610739UL,769231888UL,3831377987UL,3119803407UL,0,0,3964244169UL,193856969UL,3013031540UL,2618514484UL,3710013089UL,3854418426UL,1529296637UL,2541745584UL,0,0,3655491161UL,1467600171UL,2794150054UL,879605327UL,225093205UL,2686722043UL,1516269059UL,3935843772UL,0,0,65875932UL,3775704876UL,2369873825UL,2118503680UL,3917536027UL,536656347UL,3430166123UL,1204149331UL,0,0,1350466914UL,944908428UL,4179685299UL,1208178179UL,3977376600UL,890106164UL,284179034UL,575264655UL,0,0,},{314863038UL,3529715269UL,631795533UL,3723222638UL,1582088296UL,2549381190UL,3883494357UL,1961310067UL,0,0,2301623024UL,4200376259UL,2264010727UL,750256042UL,2229928361UL,2041958518UL,2553842869UL,2382243007UL,0,0,275676833UL,3370235705UL,976725011UL,1183391914UL,2952831812UL,627524896UL,2048095785UL,367448899UL,0,0,3242885379UL,3920309775UL,2041850855UL,1657453166UL,2465349339UL,710230352UL,655264613UL,2278559713UL,0,0,2882672253UL,599089921UL,1328843845UL,1603882997UL,3169358178UL,3148607880UL,4091105780UL,1382101854UL,0,0,3232899478UL,134376546UL,2399311620UL,2701500859UL,3807121643UL,964908323UL,3111830795UL,1951519856UL,0,0,4086050077UL,553954072UL,3646765678UL,1565159350UL,2679492810UL,3692842799UL,911967797UL,2004608239UL,0,0,4113173298UL,794163327UL,2857531285UL,4187056085UL,2619379325UL,3622304429UL,3490312864UL,3671501475UL,0,0,},{1514524451UL,1121476094UL,2847582623UL,4140267091UL,3299981106UL,2920616333UL,223151747UL,2701368070UL,0,0,151940381UL,602216656UL,1682100149UL,1911482141UL,2024725827UL,87343640UL,377040038UL,3173168997UL,0,0,4156088299UL,3587409741UL,2194021618UL,513100283UL,2640710265UL,2325745456UL,403222393UL,3346250502UL,0,0,2123102587UL,3374534947UL,3543529255UL,189792839UL,3026087644UL,2905783793UL,2499487759UL,3734694422UL,0,0,1072457040UL,3567138569UL,1919910504UL,3894081702UL,3911587566UL,2297076344UL,4264013960UL,3025345768UL,0,0,635768196UL,170904637UL,60428259UL,3921477494UL,2543376444UL,1568094829UL,2915795220UL,3429283998UL,0,0,3667548437UL,1536492985UL,378281735UL,178250244UL,898804071UL,1294986555UL,1473682048UL,790691246UL,0,0,1757813737UL,3524415542UL,4267846972UL,304951410UL,253838169UL,1963638242UL,3484183454UL,1030529913UL,0,0,},{355140392UL,1904371632UL,770053357UL,908688115UL,2919875923UL,3996613497UL,1891829673UL,288270956UL,0,0,1066908534UL,3246990931UL,943664103UL,38400997UL,2518291651UL,755232399UL,962096956UL,664809593UL,0,0,3336914151UL,4233744790UL,3807713580UL,214019571UL,3111910644UL,3072344190UL,339920652UL,4191568765UL,0,0,2105485189UL,2527910302UL,2992324049UL,1253963252UL,2253718310UL,1937657907UL,1516065979UL,582676465UL,0,0,2397054580UL,1981656584UL,3497229459UL,656829851UL,949398581UL,1748859862UL,3446715843UL,2517324041UL,0,0,372121262UL,2503680766UL,164304277UL,1830401006UL,2879526623UL,882884058UL,1824782361UL,225422966UL,0,0,2446979167UL,2733186330UL,98291562UL,1426633498UL,3389548257UL,1019127249UL,3999402385UL,1625065593UL,0,0,470599709UL,4054569190UL,1057156260UL,3205979804UL,34025082UL,2650784131UL,3571283452UL,1167261616UL,0,0,},{348728811UL,2990716609UL,419194271UL,205513054UL,3820253316UL,3666804960UL,227988099UL,4244241942UL,0,0,2618336211UL,504936615UL,106101100UL,531520028UL,1022783183UL,1226819303UL,2019658918UL,2431962963UL,0,0,3041667881UL,1239461240UL,2633094246UL,1715273024UL,1971260685UL,2190995449UL,173025498UL,3970917733UL,0,0,1968439242UL,3980798442UL,1143696626UL,2470595699UL,763506408UL,1039997815UL,2208588363UL,1494289390UL,0,0,693191163UL,3720681327UL,1906468793UL,847283140UL,1318922711UL,2817235861UL,2877301521UL,2054656835UL,0,0,1699307374UL,4258893853UL,1496241754UL,3833901747UL,3381744389UL,3161006170UL,1381684241UL,297695956UL,0,0,3600786959UL,3339723969UL,72172633UL,3434806282UL,3998977515UL,1817224287UL,2580921636UL,2733243534UL,0,0,2689418588UL,3839654532UL,66538529UL,2011720807UL,961640207UL,2692269241UL,437759033UL,1744697132UL,0,0,},},};
unsigned long Z_IS_BLACK = 2473450497UL;

/**
 * PST_OPENING/ENDGAME (Piece-Square Table)
 * └── 0,1: see enum Player
 *     └── 0...5: see enum Shape
 *         └── 0...AREA-1: see main board
 *             └── psv (Piece-Square Value) of each shape based on opening/endgame position with player at the bottom
 * 
 * values from Rofchade: http://www.talkchess.com/forum3/viewtopic.php?f=2&t=68311&start=19
 */
const short PST_OPENING[MAX_PLAYER][MAX_SHAPE][AREA] = {{{82,82,82,82,82,82,82,82,0,0,47,81,62,59,67,106,120,60,0,0,56,78,78,72,85,85,115,70,0,0,55,80,77,94,99,88,92,57,0,0,68,95,88,103,105,94,99,59,0,0,76,89,108,113,147,138,107,62,0,0,180,216,143,177,150,208,116,71,0,0,82,82,82,82,82,82,82,82,0,0,},{232,316,279,304,320,309,318,314,0,0,308,284,325,334,336,355,323,318,0,0,314,328,349,347,356,354,362,321,0,0,324,341,353,350,365,356,358,329,0,0,328,354,356,390,374,406,355,359,0,0,290,397,374,402,421,466,410,381,0,0,264,296,409,373,360,399,344,320,0,0,170,248,303,288,398,240,322,230,0,0,},{332,362,351,344,352,353,326,344,0,0,369,380,381,365,372,386,398,366,0,0,365,380,380,380,379,392,383,375,0,0,359,378,378,391,399,377,375,369,0,0,361,370,384,415,402,402,372,363,0,0,349,402,408,405,400,415,402,363,0,0,339,381,347,352,395,424,383,318,0,0,336,369,283,328,340,323,372,357,0,0,},{458,464,478,494,493,484,440,451,0,0,433,461,457,468,476,488,471,406,0,0,432,452,461,460,480,477,472,444,0,0,441,451,465,476,486,470,483,454,0,0,453,466,484,503,501,512,469,457,0,0,472,496,503,513,494,522,538,493,0,0,504,509,535,539,557,544,503,521,0,0,509,519,509,528,540,486,508,520,0,0,},{1024,1007,1016,1035,1010,1000,994,975,0,0,990,1017,1036,1027,1033,1040,1022,1026,0,0,1011,1027,1014,1023,1020,1027,1039,1030,0,0,1016,999,1016,1015,1023,1021,1028,1022,0,0,998,998,1009,1009,1024,1042,1023,1026,0,0,1012,1008,1032,1033,1054,1081,1072,1082,0,0,1001,986,1020,1026,1009,1082,1053,1079,0,0,997,1025,1054,1037,1084,1069,1068,1070,0,0,},{-15,36,12,-54,8,-28,24,14,0,0,1,7,-8,-64,-43,-16,9,8,0,0,-14,-14,-22,-46,-44,-30,-15,-27,0,0,-49,-1,-27,-39,-46,-44,-33,-51,0,0,-17,-20,-12,-27,-30,-25,-14,-36,0,0,-9,24,2,-16,-20,6,22,-22,0,0,29,-1,-20,-7,-8,-4,-38,-29,0,0,-65,23,16,-15,-56,-34,2,13,0,0,},},{{82,82,82,82,82,82,82,82,0,0,180,216,143,177,150,208,116,71,0,0,76,89,108,113,147,138,107,62,0,0,68,95,88,103,105,94,99,59,0,0,55,80,77,94,99,88,92,57,0,0,56,78,78,72,85,85,115,70,0,0,47,81,62,59,67,106,120,60,0,0,82,82,82,82,82,82,82,82,0,0,},{170,248,303,288,398,240,322,230,0,0,264,296,409,373,360,399,344,320,0,0,290,397,374,402,421,466,410,381,0,0,328,354,356,390,374,406,355,359,0,0,324,341,353,350,365,356,358,329,0,0,314,328,349,347,356,354,362,321,0,0,308,284,325,334,336,355,323,318,0,0,232,316,279,304,320,309,318,314,0,0,},{336,369,283,328,340,323,372,357,0,0,339,381,347,352,395,424,383,318,0,0,349,402,408,405,400,415,402,363,0,0,361,370,384,415,402,402,372,363,0,0,359,378,378,391,399,377,375,369,0,0,365,380,380,380,379,392,383,375,0,0,369,380,381,365,372,386,398,366,0,0,332,362,351,344,352,353,326,344,0,0,},{509,519,509,528,540,486,508,520,0,0,504,509,535,539,557,544,503,521,0,0,472,496,503,513,494,522,538,493,0,0,453,466,484,503,501,512,469,457,0,0,441,451,465,476,486,470,483,454,0,0,432,452,461,460,480,477,472,444,0,0,433,461,457,468,476,488,471,406,0,0,458,464,478,494,493,484,440,451,0,0,},{997,1025,1054,1037,1084,1069,1068,1070,0,0,1001,986,1020,1026,1009,1082,1053,1079,0,0,1012,1008,1032,1033,1054,1081,1072,1082,0,0,998,998,1009,1009,1024,1042,1023,1026,0,0,1016,999,1016,1015,1023,1021,1028,1022,0,0,1011,1027,1014,1023,1020,1027,1039,1030,0,0,990,1017,1036,1027,1033,1040,1022,1026,0,0,1024,1007,1016,1035,1010,1000,994,975,0,0,},{-65,23,16,-15,-56,-34,2,13,0,0,29,-1,-20,-7,-8,-4,-38,-29,0,0,-9,24,2,-16,-20,6,22,-22,0,0,-17,-20,-12,-27,-30,-25,-14,-36,0,0,-49,-1,-27,-39,-46,-44,-33,-51,0,0,-14,-14,-22,-46,-44,-30,-15,-27,0,0,1,7,-8,-64,-43,-16,9,8,0,0,-15,36,12,-54,8,-28,24,14,0,0,},},};
const short PST_ENDGAME[MAX_PLAYER][MAX_SHAPE][AREA] = {{{94,94,94,94,94,94,94,94,0,0,107,102,102,104,107,94,96,87,0,0,98,101,88,95,94,89,93,86,0,0,107,103,91,87,87,86,97,93,0,0,126,118,107,99,92,98,111,111,0,0,188,194,179,161,150,147,176,178,0,0,272,267,252,228,241,226,259,281,0,0,94,94,94,94,94,94,94,94,0,0,},{252,230,258,266,259,263,231,217,0,0,239,261,271,276,279,261,258,237,0,0,258,278,280,296,291,278,261,259,0,0,263,275,297,306,297,298,285,263,0,0,264,284,303,303,303,292,289,263,0,0,257,261,291,290,280,272,262,240,0,0,256,273,256,279,272,256,257,229,0,0,223,243,268,253,250,254,218,182,0,0,},{274,288,274,292,288,281,292,280,0,0,283,279,290,296,301,288,282,270,0,0,285,294,305,307,310,300,290,282,0,0,291,300,310,316,304,307,294,288,0,0,294,306,309,306,311,307,300,299,0,0,299,289,297,296,295,303,297,301,0,0,289,293,304,285,294,284,293,283,0,0,283,276,286,289,290,288,280,273,0,0,},{503,514,515,511,507,499,516,492,0,0,506,506,512,514,503,503,501,509,0,0,508,512,507,511,505,500,504,496,0,0,515,517,520,516,507,506,504,501,0,0,516,515,525,513,514,513,511,514,0,0,519,519,519,517,516,509,507,509,0,0,523,525,525,523,509,515,520,515,0,0,525,522,530,527,524,524,520,517,0,0,},{903,908,914,893,931,904,916,895,0,0,914,913,906,920,920,913,900,904,0,0,920,909,951,942,945,953,946,941,0,0,918,964,955,983,967,970,975,959,0,0,939,958,960,981,993,976,993,972,0,0,916,942,945,985,983,971,955,945,0,0,919,956,968,977,994,961,966,936,0,0,927,958,958,963,963,955,946,956,0,0,},{-53,-34,-21,-11,-28,-14,-24,-43,0,0,-27,-11,4,13,14,4,-5,-17,0,0,-19,-3,11,21,23,16,7,-9,0,0,-18,-4,21,24,27,23,9,-11,0,0,-8,22,24,27,26,33,26,3,0,0,10,17,23,15,20,45,44,13,0,0,-12,17,14,17,17,38,23,11,0,0,-74,-35,-18,-18,-11,15,4,-17,0,0,},},{{94,94,94,94,94,94,94,94,0,0,272,267,252,228,241,226,259,281,0,0,188,194,179,161,150,147,176,178,0,0,126,118,107,99,92,98,111,111,0,0,107,103,91,87,87,86,97,93,0,0,98,101,88,95,94,89,93,86,0,0,107,102,102,104,107,94,96,87,0,0,94,94,94,94,94,94,94,94,0,0,},{223,243,268,253,250,254,218,182,0,0,256,273,256,279,272,256,257,229,0,0,257,261,291,290,280,272,262,240,0,0,264,284,303,303,303,292,289,263,0,0,263,275,297,306,297,298,285,263,0,0,258,278,280,296,291,278,261,259,0,0,239,261,271,276,279,261,258,237,0,0,252,230,258,266,259,263,231,217,0,0,},{283,276,286,289,290,288,280,273,0,0,289,293,304,285,294,284,293,283,0,0,299,289,297,296,295,303,297,301,0,0,294,306,309,306,311,307,300,299,0,0,291,300,310,316,304,307,294,288,0,0,285,294,305,307,310,300,290,282,0,0,283,279,290,296,301,288,282,270,0,0,274,288,274,292,288,281,292,280,0,0,},{525,522,530,527,524,524,520,517,0,0,523,525,525,523,509,515,520,515,0,0,519,519,519,517,516,509,507,509,0,0,516,515,525,513,514,513,511,514,0,0,515,517,520,516,507,506,504,501,0,0,508,512,507,511,505,500,504,496,0,0,506,506,512,514,503,503,501,509,0,0,503,514,515,511,507,499,516,492,0,0,},{927,958,958,963,963,955,946,956,0,0,919,956,968,977,994,961,966,936,0,0,916,942,945,985,983,971,955,945,0,0,939,958,960,981,993,976,993,972,0,0,918,964,955,983,967,970,975,959,0,0,920,909,951,942,945,953,946,941,0,0,914,913,906,920,920,913,900,904,0,0,903,908,914,893,931,904,916,895,0,0,},{-74,-35,-18,-18,-11,15,4,-17,0,0,-12,17,14,17,17,38,23,11,0,0,10,17,23,15,20,45,44,13,0,0,-8,22,24,27,26,33,26,3,0,0,-18,-4,21,24,27,23,9,-11,0,0,-19,-3,11,21,23,16,7,-9,0,0,-27,-11,4,13,14,4,-5,-17,0,0,-53,-34,-21,-11,-28,-14,-24,-43,0,0,},},};

const short K_VECTOR[8] = {
    +1,             // (1, 0)
    +1 + WIDTH,     // (1, 1)
    +WIDTH,         // (0, 1)
    -1 + WIDTH,     // (-1, 1)
    -1,             // (-1, 0)
    -1 - WIDTH,     // (-1, -1)
    -WIDTH,         // (0, -1)
    +1 - WIDTH      // (1, -1)
};
const short N_VECTOR[8] = {
    2 + WIDTH,      // (2, 1)
    1 + 2*WIDTH,    // (1, 2)
   -1 + 2*WIDTH,    // (-1, 2)
   -2 + WIDTH,      // (-2, 1)
   -2 - WIDTH,      // (-2, -1)
   -1 - 2*WIDTH,    // (-1, -2)
    1 - 2*WIDTH,    // (1, -2)
    2 - WIDTH       // (2, -1)
};
const short B_VECTOR[4] = {
    +1 + WIDTH,     // down-right
    +1 - WIDTH,     // up-right
    -1 + WIDTH,     // down-left
    -1 - WIDTH      // up-left
};
const short R_VECTOR[4] = {
    +1,             // right
    -1,             // left
    +WIDTH,         // down
    -WIDTH          // up
};
// no Q_VECTOR since it's a combination of B_VECTOR & R_VECTOR

struct BoardEntry
{
    Player player;
    Shape shape;
    short sq;

    BoardEntry(Player player = BLACK, Shape shape = PAWN, short sq = -1) : player(player), shape(shape), sq(sq)
    {}
};
const size_t TABLE_SZ = 2048; // must be power of 2
struct TtableEntry
{
    unsigned long hash;
    short score;
    short game_depth;

    TtableEntry () : hash(0), score(0), game_depth(0)
    {}
};

short x_of(short sq)
{
    return sq % WIDTH;
}

short y_of(short sq)
{
    return sq / WIDTH;
}

bool is_play_area(short sq)
{
    return unsigned(sq) < AREA && x_of(sq) < PLAY_WIDTH; // if x < 0, it becomes a huge unsigned number > AREA.
}

/**
 * @return the forward direction relative to the given player.
 * With BLACK at the top and WHITE at the bottom, black's forward = +WIDTH, white's forward = -WIDTH.
 */
short rel_forward(Player player)
{
    return (player) ? -WIDTH : WIDTH;
}

struct Engine
{
    class MVVLVAMoveGenerator
    {
        public:
            short sq_i = 0, sq_f = 0;
            Shape shape_a;
            MVVLVAMoveGenerator(Player player, const Engine &engine) : engine(engine)
            {
                gen_MVVLVA_moves(player);
            }

            /**
             * @return whether moves[] has element to assign to shape_a, sq_i, sq_f.
             */
            bool next()
            {
                while (i_v < KING)
                {
                    while (i_a < MAX_SHAPE)
                    {
                        if (i + 1 < moves_end[i_v][i_a])
                        {
                            shape_a = Shape(i_a);
                            short *arr = moves[i_v][i_a];
                            sq_i = arr[i++];
                            sq_f = arr[i++];
                            return true;
                        }
                        i = 0;
                        i_a++;
                    }
                    i_a = 0;
                    i_v++;
                }
                if (i + 2 < quiet_moves.size())
                {
                    shape_a = Shape(quiet_moves[i++]);
                    sq_i = quiet_moves[i++];
                    sq_f = quiet_moves[i++];
                    return true;
                }
                return false;
            };

        private:
            /**
             * moves
             * └── victims ordered from most to least valuable: QUEEN, ..., PAWN (KING cannot be captured)
             *     └── attackers ordered from least to most valuable: PAWN, ..., KING
             *         └── sq_i,sq_f, sq_i,sq_f, ...
             * 
             * quiet_moves has - shape,sq_i,sq_f, shape,sq_i,sq_f, ...
             */
            short moves[KING][MAX_SHAPE][MAX_2NUM_AXB] = {{{0}}}, moves_end[KING][MAX_SHAPE] = {{0}}, i_v = 0, i_a = 0, i = 0;
            const Engine &engine;
            std::vector<short> quiet_moves;

            void MVVLVA_insert(Shape shape_v)
            {
                short i_v = QUEEN - shape_v;
                short *my_moves = moves[i_v][shape_a];
                short &my_moves_end = moves_end[i_v][shape_a];
                my_moves[my_moves_end++] = sq_i;
                my_moves[my_moves_end++] = sq_f;
            }
            void quiet_push()
            {
                short quiet_moves_end = quiet_moves.size();
                quiet_moves.resize(quiet_moves_end + 3);
                quiet_moves[quiet_moves_end] = shape_a;
                quiet_moves[quiet_moves_end + 1] = sq_i;
                quiet_moves[quiet_moves_end + 2] = sq_f;
            }

            void gen_MVVLVA_moves(Player player)
            {
                quiet_moves.reserve(MAX_3NUM_AXB);
                for (const BoardEntry *sq_i_ptr = engine.pieces[player]; sq_i_ptr <= engine.KING_IND[player]; sq_i_ptr++)
                {
                    sq_i = sq_i_ptr->sq;
                    if (sq_i >= 0) // if not captured
                    {
                        shape_a = sq_i_ptr->shape;
                        if (shape_a == PAWN)
                        {
                            short y_i = y_of(sq_i);
                            if (1 <= y_i && y_i <= 6)
                            {
                                short dy = rel_forward(player), sq_y = sq_i + dy;
                                bool is_promote = (y_i == 1 && dy < 0) || (y_i == 6 && dy > 0);

                                // capture moves
                                for (short dx = -1; dx <= 1; dx += 2)
                                {
                                    sq_f = sq_y + dx;
                                    if (is_play_area(sq_f) && engine.squares[sq_f])
                                    {
                                        const BoardEntry &piece = *engine.squares[sq_f];
                                        const Shape shape_v = piece.shape;      
                                        if (piece.player != player && shape_v != KING)
                                        {
                                            if (is_promote)
                                                MVVLVA_insert(QUEEN); // capture promotion best, treat as PAWN x QUEEN
                                            else
                                                MVVLVA_insert(shape_v);
                                        }
                                    }
                                }

                                // y-moves
                                sq_f = sq_y;
                                if (!engine.squares[sq_f])
                                {
                                    if (is_promote)
                                        MVVLVA_insert(ROOK); // quiet promotion 2nd best, treat as PAWN x ROOK
                                    else
                                    {
                                        quiet_push();

                                        if ((y_i == 1 && dy > 0) || (y_i == 6 && dy < 0)) // can step 2
                                        {
                                            sq_f += dy;
                                            if (!engine.squares[sq_f])
                                                quiet_push();
                                        }
                                    }
                                }
                            }
                        }
                        else if (shape_a == KNIGHT)
                        {
                            for (short v: N_VECTOR)
                            {
                                sq_f = sq_i + v;
                                if (is_play_area(sq_f))
                                {
                                    if (!engine.squares[sq_f])
                                        quiet_push();
                                    else
                                    {
                                        const BoardEntry &piece = *engine.squares[sq_f];
                                        const Shape shape_v = piece.shape;
                                        if (piece.player != player && shape_v != KING)
                                            MVVLVA_insert(shape_v);
                                    }
                                }
                            }
                        }
                        else if (shape_a == KING)
                        {
                            for (short v: K_VECTOR)
                            {
                                sq_f = sq_i + v;
                                if (is_play_area(sq_f))
                                {
                                    if (!engine.squares[sq_f])
                                        quiet_push();
                                    else
                                    {
                                        const BoardEntry &piece = *engine.squares[sq_f];
                                        const Shape shape_v = piece.shape;
                                        if (piece.player != player && shape_v != KING)
                                            MVVLVA_insert(shape_v);
                                    }
                                }
                            }
                        }
                        else
                        {
                            if (shape_a == BISHOP || shape_a == QUEEN)
                            {
                                for (short v: B_VECTOR)
                                {
                                    for (sq_f = sq_i + v; is_play_area(sq_f) && !engine.squares[sq_f]; sq_f += v)
                                        quiet_push();
                                    
                                    if (is_play_area(sq_f))
                                    {
                                        const BoardEntry &piece = *engine.squares[sq_f];
                                        const Shape shape_v = piece.shape;
                                        if (piece.player != player && shape_v != KING)
                                            MVVLVA_insert(shape_v);
                                    }
                                }
                            }
                            if (shape_a == ROOK || shape_a == QUEEN)
                            {
                                for (short v: R_VECTOR)
                                {
                                    // travel until on top of a piece
                                    for (sq_f = sq_i + v; is_play_area(sq_f) && !engine.squares[sq_f]; sq_f += v)
                                        quiet_push();

                                    if (is_play_area(sq_f))
                                    {
                                        const BoardEntry &piece = *engine.squares[sq_f];
                                        const Shape shape_v = piece.shape;
                                        if (piece.player != player && shape_v != KING)
                                            MVVLVA_insert(shape_v);
                                    }
                                }
                            }
                        }
                    }
                }
            }
    };

    /**
     * pieces
     * └── 0,1: see enum Player
     *     └── 0...16: max total number of pieces of each player
     *         └── piece: { player, shape, sq }
     * 
     * If an piece is captured, its sq = sq - AREA.
     * 
     * IMPORTANT: Custom board sizes and positions are supported, but with the following constraints:
     * 1. First element must be PAWN. Last element must be KING.
     * 2. Each shape has a maximum of 8 pieces.
     * 3. Remember to change MAX_NUM_PIECE.
     */
    BoardEntry pieces[MAX_PLAYER][MAX_NUM_PIECE] = {
        {   // black
            { BLACK, PAWN,  10 }, { BLACK, PAWN,  11 }, { BLACK, PAWN, 12 }, { BLACK, PAWN, 13 }, { BLACK, PAWN, 14 }, { BLACK, PAWN, 15 }, { BLACK, PAWN, 16 }, { BLACK, PAWN, 17 },
            { BLACK, KNIGHT, 1 }, { BLACK, KNIGHT, 6 },
            { BLACK, BISHOP, 2 }, { BLACK, BISHOP, 5 },
            { BLACK, ROOK,   0 }, { BLACK, ROOK,   7 },
            { BLACK, QUEEN,  3 },
            { BLACK, KING,   4 }
        },
        {   // white
            { WHITE, PAWN,   60 }, { WHITE, PAWN,   61 }, { WHITE, PAWN, 62 }, { WHITE, PAWN, 63 }, { WHITE, PAWN, 64 }, { WHITE, PAWN, 65 }, { WHITE, PAWN, 66 }, { WHITE, PAWN, 67 },
            { WHITE, KNIGHT, 71 }, { WHITE, KNIGHT, 76 },
            { WHITE, BISHOP, 72 }, { WHITE, BISHOP, 75 },
            { WHITE, ROOK,   70 }, { WHITE, ROOK,   77 },
            { WHITE, QUEEN,  73 },
            { WHITE, KING,   74 }
        }
    };
    const BoardEntry *KING_IND[MAX_PLAYER] = { &pieces[BLACK][MAX_NUM_PIECE-1], &pieces[WHITE][MAX_NUM_PIECE-1] };

    /** 
     * squares
     * ├── columns 8,9 element
     * │   └── 0ptr as sentinel
     * │
     * └── columns 0...8 element
     *     └── pointer to a piece in the pieces[], else 0ptr as empty square.
     * 
     * The squares is 10x8 since the two rightmost columns are sentinels that prevent
     * iterators from "wrapping" onto the previous/next row. Sentinels/empties are 0ptr to identify easily using !squares[sq].
     */
    BoardEntry *squares[AREA] = {nullptr};

    unsigned long hash = 0;

    TtableEntry t_table[TABLE_SZ]; // Transposition Table

    unsigned long move_history[MAX_DEPTH] = {0};

    short root_depth = 1; // move counter. Only update when bot move.
    short MAX_PHASE = 0; // sum of SHAPE_PHASE of all initial pieces
    short phase = 0; // sum of SHAPE_PHASE of all current pieces
    short psv_opening[MAX_PLAYER] = {0, 0};
    short psv_endgame[MAX_PLAYER] = {0, 0};

    Engine()
    {
        for (short player = BLACK; player <= WHITE; player++)
        {
            // NBRQ_BEGIN, NBRQ_END, squares, hash, MAX_PHASE, phase, psv_opening, psv_endgame
            for (BoardEntry &piece : pieces[player])
            {
                short sq = piece.sq;
                if (sq >= 0)
                {
                    const Shape &shape = piece.shape;

                    if (shape == KING)
                        KING_IND[player] = &piece;

                    squares[sq] = &piece;
                    hash ^= ZTABLE[player][shape][sq];
                    phase += SHAPE_PHASE[shape];
                    psv_opening[player] += PST_OPENING[player][shape][sq];
                    psv_endgame[player] += PST_ENDGAME[player][shape][sq];
                }
            }
        }
        MAX_PHASE = phase;
    }

    void add_psv(Player player, Shape shape, short sq)
    {
        psv_opening[player] += PST_OPENING[player][shape][sq];
        psv_endgame[player] += PST_ENDGAME[player][shape][sq];
    }

    void del_psv(Player player, Shape shape, short sq)
    {
        psv_opening[player] -= PST_OPENING[player][shape][sq];
        psv_endgame[player] -= PST_ENDGAME[player][shape][sq];
    }

    void move(unsigned long &my_hash, bool &is_promote, Player player, Shape shape, short sq_i, short sq_f)
    {
        BoardEntry *sq_i_ptr = squares[sq_i];
        BoardEntry *sq_f_ptr = squares[sq_f];

        // capture
        if (sq_f_ptr)
        {
            Player del_player = sq_f_ptr->player;
            Shape del_shape = sq_f_ptr->shape;

            sq_f_ptr->sq -= AREA; // move sq outside board
            phase -= SHAPE_PHASE[del_shape];
            del_psv(del_player, del_shape, sq_f);
            my_hash ^= ZTABLE[del_player][del_shape][sq_f];
        }
        sq_i_ptr->sq = sq_f;
        del_psv(player, shape, sq_i);
        my_hash ^= ZTABLE[player][shape][sq_i] ^ Z_IS_BLACK;

        // promotion
        if (shape == PAWN && (sq_f < WIDTH || sq_f >= AREA-WIDTH))
        {
            sq_i_ptr->shape = QUEEN;
            phase += SHAPE_PHASE[QUEEN]; // no need to subtract SHAPE_PHASE[PAWN] that is 0
            add_psv(player, QUEEN, sq_f);
            my_hash ^= ZTABLE[player][QUEEN][sq_f];
            is_promote = true;
        }
        else
        {
            add_psv(player, shape, sq_f);
            my_hash ^= ZTABLE[player][shape][sq_f];
            is_promote = false;
        }

        squares[sq_f] = sq_i_ptr;
        squares[sq_i] = nullptr;
    }

    /**
     * IMPORTANT:
     * 1. Doesn't undo child_hash.
     * 2. Same sq_i & sq_f as doing move.
     * 3. Same shape as before promotion (promoted QUEEN still has shape = PAWN).
     */
    void unmove(bool is_promote, Player player, Shape shape, short sq_i, short sq_f, BoardEntry *prev_sq_f_ptr)
    {
        BoardEntry *sq_f_ptr = squares[sq_f];
        sq_f_ptr->sq = sq_i;
        add_psv(player, shape, sq_i);

        // demotion
        if (is_promote)
        {
            sq_f_ptr->shape = PAWN;
            phase -= SHAPE_PHASE[QUEEN]; // no need to add SHAPE_PHASE[PAWN] that is 0
            del_psv(player, QUEEN, sq_f);
        }
        else
            del_psv(player, shape, sq_f);

        // restore capture
        if (prev_sq_f_ptr)
        {
            Player add_player = prev_sq_f_ptr->player;
            Shape add_shape = prev_sq_f_ptr->shape;

            prev_sq_f_ptr->sq += AREA; // move sq back inside
            phase += SHAPE_PHASE[add_shape];
            add_psv(add_player, add_shape, sq_f);
        }

        squares[sq_i] = sq_f_ptr;
        squares[sq_f] = prev_sq_f_ptr;
    }

    /**
     * @return whether all entries on the path are empty.
     */
    bool is_path_clear(short sq_i, short sq_f, unsigned short dx, unsigned short dy)
    {
        short v = (sq_f - sq_i) / std::max(dx, dy);
        for (short sq = sq_i + v; sq != sq_f; sq += v)
        {
            if (squares[sq])
                return false;
        }
        return true;
    }

    /**
     * @return whether the given square is currently attacked by the enemy of the given player.
     */
    bool is_checked(Player player)
    {
        // check for enemy pawns by posing as pawn and check where I can capture
        short dx = 0, dy = 0, sq_a = 0, sq_k = KING_IND[player]->sq;
        {
            short sq_y = sq_k + rel_forward(player);
            for (dx = -1; dx <= 1; dx += 2)
            {
                sq_a = sq_y + dx;
                if (is_play_area(sq_a) && squares[sq_a])
                {
                    const BoardEntry &attacker = *squares[sq_a];
                    if (attacker.player != player && attacker.shape == PAWN)
                        return true;
                }
            }
        }

        // check for enemy KNIGHT, BISHOP, ROOK, QUEEN, KING by iterating over each piece
        Shape shape_a;
        for (BoardEntry *attacker = pieces[!player]; attacker <= KING_IND[!player]; attacker++)
        {
            sq_a = attacker->sq;
            shape_a = attacker->shape;
            if (shape_a != PAWN && sq_a >= 0)
            {
                dx = abs(x_of(sq_a) - x_of(sq_k));
                dy = abs(y_of(sq_a) - y_of(sq_k));
                if (shape_a == KNIGHT && ((dx == 1 && dy == 2) || (dx == 2 && dy == 1)))
                    return true;

                else if (shape_a == KING && std::max(dx, dy) <= 1)
                    return true;

                else if (shape_a == BISHOP && dx == dy && is_path_clear(sq_k, sq_a, dx, dy))
                    return true;

                else if (shape_a == ROOK && (dx == 0 || dy == 0) && is_path_clear(sq_k, sq_a, dx, dy))
                    return true;

                else if (shape_a == QUEEN && (dx == dy || dx == 0 || dy == 0) && is_path_clear(sq_k, sq_a, dx, dy))
                    return true;
            }
        }
        return false;
    }

    short static_eval()
    {
        short score_opening = psv_opening[MAXER] - psv_opening[MINER],
            score_endgame = psv_endgame[MAXER] - psv_endgame[MINER];

        return score_endgame - (score_endgame - score_opening)*std::min(phase, MAX_PHASE)/MAX_PHASE; // linear interpolation
    }

    bool is_repeat(const unsigned long &my_hash, short depth)
    {
        return (depth >= 3 && my_hash == move_history[depth-3]); // experimentally determined no need to check lower/higher depths
    }

    void into_t_table(const unsigned long &my_hash, short score, short game_depth)
    {
        TtableEntry &entry = t_table[my_hash % TABLE_SZ];
        if (game_depth >= entry.game_depth) // if bucket collision, keep the more recent and deeper search
        {
            entry.hash = my_hash;
            entry.score = score;
            entry.game_depth = game_depth;
        }
    }

    short worst_score(Player player, short depth)
    {
        return (player == MAXER) ? SHRT_MIN + depth : SHRT_MAX - depth; // earlier checkmate worst than later
    }

    /**
     * Minimax + Tapered Piece-Square Table Evaluation + AlphaBeta Pruning + 0-Move Prunning + Zobrist Hashing Transposition Table + MVV-LVA + Repetition Check + Quiescence Search + Standing Pat
     * In first call, use depth = 1, alpha = SHRT_MIN, beta = SHRT_MAX.
     * @return
     *      n ∈ (SHRT_MIN, 0) U (0, SHRT_MAX) if over MAX_DEPTH.
     *      n = SHRT_MAX if check/stalemated by MAXER.
     *      n = SHRT_MIN if check/stalemated by MINER.
     */
    short eval(unsigned long my_hash, Player player, short depth, short alpha, short beta, bool is_NM_eval, bool is_PV_node)
    {
        if (depth == MAX_DEPTH)
            return static_eval();

        move_history[depth] = my_hash;
        bool is_checked_i = is_checked(player);
        short child_score = 0;

        // Null-Move Pruning
        if (depth <= MAX_NM_DEPTH && phase && !is_NM_eval && !is_PV_node && !is_checked_i)
        {
            if (player == MAXER)
            {
                child_score = eval(my_hash^Z_IS_BLACK, !player, depth + NM_DEPTH_INC, beta-1, beta, true, false);
                if (child_score >= beta)
                {
                    move_history[depth] = 0;
                    return beta;
                }
            }
            else
            {
                child_score = eval(my_hash^Z_IS_BLACK, !player, depth + NM_DEPTH_INC, alpha, alpha+1, true, false);
                if (child_score <= alpha)
                {
                    move_history[depth] = 0;
                    return alpha;
                }
            }
        }

        bool is_promote = 0, has_child_score = 0;
        short my_worst_score = worst_score(player, depth), score = my_worst_score, game_depth = 0;
        unsigned long child_hash = 0;
        BoardEntry *sq_f_ptr;
        MVVLVAMoveGenerator moves(player, *this);
        while (moves.next())
        {
            child_hash = my_hash;
            has_child_score = false;
            sq_f_ptr = squares[moves.sq_f];
            move(child_hash, is_promote, player, moves.shape_a, moves.sq_i, moves.sq_f);

            const TtableEntry &child_entry = t_table[child_hash % TABLE_SZ];
            game_depth = root_depth + depth;
            if (child_hash == child_entry.hash && child_entry.game_depth >= game_depth)
            {
                child_score = child_entry.score;
                has_child_score = true;
            }
            else if (!is_repeat(child_hash, depth) && !is_checked(player))
            {
                child_score = eval(child_hash, !player, depth+1, alpha, beta, is_NM_eval, is_PV_node);
                has_child_score = true;
                is_PV_node = false;
            }

            unmove(is_promote, player, moves.shape_a, moves.sq_i, moves.sq_f, sq_f_ptr);

            if (has_child_score)
            {
                if (player == MAXER)
                {
                    if (child_score > score)
                        score = child_score;
                    if (child_score > alpha)
                        alpha = child_score;
                }
                else
                {
                    if (child_score < score)
                        score = child_score;
                    if (child_score < beta)
                        beta = child_score;
                }
                if (alpha >= beta)
                {
                    into_t_table(my_hash, score, game_depth);
                    move_history[depth] = 0;
                    return score;
                }
            }
        }
        move_history[depth] = 0;
        if (score == my_worst_score && !is_checked_i) // stalemate
            score = 0;
        into_t_table(my_hash, score, game_depth);
        moveTouchLED.on(std::min(std::max(0, score+90), 180), 100);
        return score;
    }

    /**
     * @return
     *      mate_type = 0 if game continues.     
     *      mate_type = 1 if bot is checkmated.
     *      mate_type = 2 if bot is stalemated.
     */
    void bot_move(short &best_sq_i, short &best_sq_f, short &mate_type)
    {
        move_history[0] = hash;

        bool is_checked_i = is_checked(BLACK), is_PV_node = true, is_promote = 0, has_child_score = 0;
        short child_score = 0, my_worst_score = worst_score(BLACK, 0), score = my_worst_score;
        unsigned long child_hash = 0;
        BoardEntry *sq_f_ptr;
        MVVLVAMoveGenerator moves(BLACK, *this);
        printf("Possible {square_i, square_f, score}:\n");
        while (moves.next() && score != worst_score(WHITE, 0))
        {
            child_hash = hash;
            has_child_score = false;
            sq_f_ptr = squares[moves.sq_f];
            move(child_hash, is_promote, BLACK, moves.shape_a, moves.sq_i, moves.sq_f);
            
            if (!is_checked(BLACK))
            {
                child_score = eval(child_hash, WHITE, 1, SHRT_MIN, SHRT_MAX, false, is_PV_node);
                has_child_score = true;
                is_PV_node = false;
            }

            unmove(is_promote, BLACK, moves.shape_a, moves.sq_i, moves.sq_f, sq_f_ptr);

            if (has_child_score)
            {
                printf("{%d, %d, %d}\n", moves.sq_i, moves.sq_f, child_score);
                if (child_score < score)
                {
                  Brain.playNote(1, 0);
                  best_sq_i = moves.sq_i;
                  best_sq_f = moves.sq_f;
                  score = child_score;
                }
                else
                  Brain.playNote(7, 0);
            }
        }
        if (score == my_worst_score)
        {
            if (!is_checked_i) // stalemated
                mate_type = 2;
            else
                mate_type = 1; // checkmated
        }
        mate_type = 0;
        root_depth += 2; // +1 for human, +1 for bot
    }

    /**
     * @return whether move is valid.
     * 0: valid. 1: illegal move vector. 2: outside play area. 3: empty initial square. 4: impersonating bot. 5: illegal capture. 6: blocked path. 7: is in check
     */
    short validate(short sq_i, short sq_f)
    {
        if (!is_play_area(sq_i) || !is_play_area(sq_f))
            return 2;

        BoardEntry *sq_i_ptr = squares[sq_i], *sq_f_ptr = squares[sq_f];
        if (!sq_i_ptr)
            return 3;

        Player player = sq_i_ptr->player;
        Shape shape = sq_i_ptr->shape;

        if (player == BLACK)
            return 4;

        else if (sq_f_ptr && (sq_f_ptr->player == player || sq_f_ptr->shape == KING))
            return 5;

        short dx = abs(x_of(sq_f) - x_of(sq_i)),
            dy = abs(y_of(sq_f) - y_of(sq_i));

        if (shape == PAWN)
        {
            if (rel_forward(player)*(sq_f - sq_i) < 0) // moved backward
                return 1;

            if (!sq_f_ptr)
            {
                if (dx != 0)
                    return 1;

                if (dy > 1 + (y_of(sq_i) == 1 || y_of(sq_i) == 6)) // if pawn at starting line dy > 1+(1), else dy>1+(0)
                    return 1;
            }
            else if (dx != 1 || dy != 1) // && not empty
                return 1;
        }
        else if (shape == KNIGHT && (dx != 1 || dy != 2) && (dx != 2 || dy != 1))
            return 1;

        else if (shape == BISHOP && dx != dy)
            return 1;

        else if (shape == ROOK && dx && dy)
            return 1;

        else if (shape == QUEEN && dx && dy && dx != dy)
            return 1;

        else if (shape == KING && std::max(dx, dy) > 1)
            return 1;

        if (shape != KNIGHT && shape != KING && !is_path_clear(sq_i, sq_f, dx, dy))
            return 6;

        bool is_promote = 0;
        unsigned long _ = 0;
        move(_, is_promote, player, shape, sq_i, sq_f);
        if (is_checked(player))
        {
            unmove(is_promote, player, shape, sq_i, sq_f, sq_f_ptr);
            return 7;
        }
        unmove(is_promote, player, shape, sq_i, sq_f, sq_f_ptr);

        // if all valid
        return 0;
    }
};
/*
  ---------- END OF CHESSBOT CODE ---------
*/



/*
  ---------- START OF GANTRY MOVEMENT CODE ---------
*/

void zero_xyz();     

void print_square();

void get_curr_square(short& square_x, short& square_y);

void move_gantry_x(short squares_moved); // Moves in x direction by a number of squares

void move_gantry_y(short squared_moved); // moves in y

void pid_rotate_degrees(motor&, double, const double, const double);

void reset_all_motors();

void move_to(short to_x, short to_y);

void capture(short x, short y);

void pick_up();

void release();

// double min(double x, double y){
//   return (x < y) ? x : y;
// }

// double max(double x, double y){
//   return (x > y) ? x : y;
// }

double clamp(double min, double val, double max){

  if(val < min || val > max){
    return (val > max) ? max : min; 
  }

  return val;
}

int iround(double d){
  return (d > 0) ? d + 0.5 : d - 0.5 ;
}

const double DEGREES_SQUARE_X = 321.37; // Empirically found
const double DEGREES_SQUARE_Y = 331.45; // Empirically found

const double ZEROED_X_INITIAL = 205; // Position of first square after zeroing x
const double ZEROED_Y_INITIAL = 318; // Position of first square after zeroing x
const double ZEROED_Z_INITIAL = 50; // Position of first square after zeroing x   

const double Z_TOLERANCE = 0;
const double X_TOLERANCE = 35;
const double Y_TOLERANCE = 45;
const double P_DEFAULT = 0.5;

const double MAX_SPEED = 50;
const double MIN_SPEED = 1;
const double ZERO_SPEED = 40;

const double MIN_Y = 0;
const double MIN_X = 0;
const double MAX_Y = 9;
const double MAX_X = 7;
const double CAPTURE_Y = 9;

const double MAX_MOTOR_CURRENT = 0.3;

void pid_rotate_degrees(motor& Motor, double deg, double TOLERANCE, double P_SCALAR){

  vex::directionType direction = forward;
  const double original_pos = Motor.position(degrees);

  if(abs(deg) <= TOLERANCE)
    return;
  
  if(deg < 0)
    direction = reverse;

  double distance_from = 0;
  
  while(distance_from <= (abs(deg) - TOLERANCE) && Motor.current(amp) < MAX_MOTOR_CURRENT)
   {
    double proportional = P_SCALAR * (abs(deg) - distance_from);

    Motor.spin(direction, clamp(MIN_SPEED, proportional, MAX_SPEED), percent);
    // This is intentional. It slows down as it approached the correct position.
    wait(1, msec);
    distance_from = abs(Motor.position(degrees) - original_pos);
  }

  Motor.stop(brake);
}

void zero_xyz(){
  
  xAxisMotor.spin(reverse, ZERO_SPEED, percent);
  while(xAxisMotor.current() < MAX_MOTOR_CURRENT){}
  xAxisMotor.stop(coast);   
  xAxisMotor.resetPosition();

  yAxisMotor.spin(reverse, ZERO_SPEED, percent);
  while(yAxisMotor.current() < MAX_MOTOR_CURRENT){}
  yAxisMotor.stop(coast);
  yAxisMotor.resetPosition();

  zAxisMotor.spin(reverse, ZERO_SPEED, percent);
  while(!zAxisBumper.pressing() && zAxisMotor.current() < MAX_MOTOR_CURRENT){}
  zAxisMotor.stop(brake);
  zAxisMotor.resetPosition();

  pid_rotate_degrees(zAxisMotor, ZEROED_Z_INITIAL, Z_TOLERANCE, P_DEFAULT);
  pid_rotate_degrees(xAxisMotor, ZEROED_X_INITIAL, X_TOLERANCE, P_DEFAULT);
  pid_rotate_degrees(yAxisMotor, ZEROED_Y_INITIAL, Y_TOLERANCE, P_DEFAULT);

  Brain.Screen.clearScreen();
  Brain.Screen.setCursor(1,1);
  Brain.Screen.print("Zeroing Complete.");

  wait(2, seconds);

  Brain.Screen.clearScreen();

  reset_all_motors();
}

void reset_all_motors(){
  zAxisMotor.resetPosition();
  yAxisMotor.resetPosition();
  xAxisMotor.resetPosition();
}

void get_curr_square(short& x, short& y){
  
  x = iround(xAxisMotor.position(degrees) / DEGREES_SQUARE_X);
  y = iround(yAxisMotor.position(degrees) / DEGREES_SQUARE_Y);

}
                                    
void move_gantry_x(short squares_moved){
  
  short x = 0, y = 0;
  get_curr_square(x, y);

  double desired_x = x + squares_moved;

  if (desired_x < MIN_X || desired_x > MAX_X){
    return;
  }

  double degrees_moved = (desired_x * DEGREES_SQUARE_X) - xAxisMotor.position(degrees);

  pid_rotate_degrees(xAxisMotor, degrees_moved , Z_TOLERANCE, P_DEFAULT);
} 

void move_gantry_y(short squares_moved){

  short x = 0, y = 0;
  get_curr_square(x, y);

  double desired_y = y + squares_moved;

  if (desired_y < MIN_Y || desired_y > MAX_Y){
    return;
  }

  double degrees_moved = (desired_y * DEGREES_SQUARE_Y) - yAxisMotor.position(degrees);

  pid_rotate_degrees(yAxisMotor, degrees_moved , Z_TOLERANCE, P_DEFAULT);
}

void move_to(short to_x, short to_y){

  if(MIN_X > to_x || to_x > MAX_X || MIN_Y > to_y || to_y > MAX_Y)
    return;

  short x = 0, y = 0;
  get_curr_square(x, y);

  move_gantry_x(to_x - x);
  move_gantry_y(to_y - y);

}

void capture(short x, short y){

  move_to(x, y);
  pick_up();
  move_to(x, CAPTURE_Y);
  release();

}

/*
  ---------- END OF GANTRY MOVEMENT CODE ---------
*/



/*
  ---------- START OF USER INPUT CODE ---------
*/

/*
The functions resetOutput, inputValue, and convertTo1D are necessary for wait_for_move to work
wait_for_move is a void function that accepts the engine as a pass by reference parameter
*/

// GLOBAL CONSTANTS
const short MAX_SQ = PLAY_WIDTH - 1;

void resetOutput()
{
    Brain.Screen.clearScreen();
    Brain.Screen.setCursor(1, 1);
}

short inputValue(bool isX, bool isInitial)
{
  short sq = 0;

  while(!Brain.buttonCheck.pressing())
  {
    resetOutput();
    if (isX)
      if (isInitial)
        Brain.Screen.print("Select initial x:");
      else
        Brain.Screen.print("Select final x:");
    else
      if (isInitial)
        Brain.Screen.print("Select initial y:");
      else
        Brain.Screen.print("Select final y:");
    Brain.Screen.newLine();
    Brain.Screen.print("%d", sq);

    while(!(Brain.buttonLeft.pressing() || Brain.buttonRight.pressing() || Brain.buttonCheck.pressing()))
    // Assume that only one button is pressed at a time
    {}
    if (Brain.buttonRight.pressing())
    {
      while(Brain.buttonRight.pressing())
      {}
      if (sq < MAX_SQ)
        sq++;
      else
        sq = 0;
    }

    if (Brain.buttonLeft.pressing())
    {
      while(Brain.buttonLeft.pressing())
      {}
      if (sq > 0)
        sq--;
      else
        sq = MAX_SQ;
    }
  }
  while(Brain.buttonCheck.pressing())
  {}

  return sq;
}

short convertTo1D(short x, short y)
{
  return (WIDTH * y + x);
}

void wait_for_move(Engine & engine, short & sq_ix, short & sq_iy, short & sq_fx, short & sq_fy, short & sq_i, short & sq_f, bool & isCheckmate)
{
  short errorType = -1;

  while (errorType != 0)
  {
    bool cancelMove = false;

    do
    {
      cancelMove = false;  

      sq_ix = 0;
      sq_iy = 0;
      sq_fx = 0;
      sq_fy = 0;

      sq_ix = inputValue(true,true);
      sq_iy = inputValue(false,true);
      sq_fx = inputValue(true,false);
      sq_fy = inputValue(false,false);

      resetOutput();
      Brain.Screen.print("tchLED = confirm");
      Brain.Screen.newLine();
      Brain.Screen.print("check = cancel");
      Brain.Screen.newLine();
      Brain.Screen.print("left = checkmate");
      Brain.Screen.newLine();
      Brain.Screen.print("Initial: (%d, %d)", sq_ix, sq_iy);
      Brain.Screen.newLine();
      Brain.Screen.print("Final: (%d, %d)", sq_fx, sq_fy);

      while(!(moveTouchLED.pressing() || Brain.buttonCheck.pressing() || Brain.buttonLeft.pressing()))
      {}
      if (Brain.buttonCheck.pressing())
        cancelMove = true;
      if (Brain.buttonLeft.pressing())
      {
        isCheckmate = true;
        errorType = 0;
      }
      while (moveTouchLED.pressing() || Brain.buttonCheck.pressing() || Brain.buttonLeft.pressing())
      {}

    } while(cancelMove);

    if (!isCheckmate)
    {
      sq_i = convertTo1D(sq_ix, sq_iy);
      sq_f = convertTo1D(sq_fx, sq_fy);

      errorType = engine.validate(sq_i, sq_f);

      if (errorType != 0)
      {
        moveTouchLED.setColor(red);
        resetOutput();
        Brain.Screen.print("Invalid Move!");
        Brain.Screen.newLine();

        /*
          1: Illegal move vector
          2: Outside play area
          3: Empty initial square
          4: Impersonating bot
          5: Illegal capture
          6: Blocked path
          7: Is in check
        */

        if (errorType == 1)
        {
          Brain.Screen.print("Illegal move");
          Brain.Screen.newLine();
          Brain.Screen.print("vector");
        }
        else if (errorType == 2)
        {
          Brain.Screen.print("Outside play");
          Brain.Screen.newLine();
          Brain.Screen.print("area");
        }
        else if (errorType == 3)
        {
          Brain.Screen.print("Empty initial");
          Brain.Screen.newLine();
          Brain.Screen.print("square");
        }
        else if (errorType == 4)
        {
          Brain.Screen.print("Impersonating");
          Brain.Screen.newLine();
          Brain.Screen.print("bot");
        }
        else if (errorType == 5)
          Brain.Screen.print("Illegal capture");
        else if (errorType == 6)
          Brain.Screen.print("Blocked path");
        else
          Brain.Screen.print("Is in check");

        wait(3, seconds);
        moveTouchLED.off();
      }
      else
      {
        moveTouchLED.setColor(green);
        wait(0.5, seconds);
        moveTouchLED.off();
      }
    } 
  }
}

// Converts 2D coordinate system from perspective of chessboard to gantry
void convertToGantry(short& x, short& y)
{
  y = MAX_SQ - y;
}

/*
  ---------- END OF USER INPUT CODE ---------
*/



/*
  ---------- START OF PICKUP AND RELEASE CODE ---------
*/

const double PICK_UP = 70, RELEASE = 0, DROP_TO_PICK = 485;

void pick_up()
{
  double deg_down = DROP_TO_PICK - zAxisMotor.position(degrees);
  pid_rotate_degrees(zAxisMotor, deg_down, Z_TOLERANCE, P_DEFAULT);

  wait(1,seconds);

  double deg_up = PICK_UP - zAxisMotor.position(degrees);
  pid_rotate_degrees(zAxisMotor, deg_up, Z_TOLERANCE, P_DEFAULT);
}

void release()
{
  double deg_up = RELEASE - zAxisMotor.position(degrees);
  pid_rotate_degrees(zAxisMotor, deg_up, Z_TOLERANCE, P_DEFAULT);
}

void check_and_capture(const Engine& engine, short sq_fx, short sq_fy){
    if(engine.squares[convertTo1D(sq_fx, sq_fy)]) // Change x and y to correct variables
  {
    convertToGantry(sq_fx, sq_fy);
    capture(sq_fx, sq_fy);
    convertToGantry(sq_fx, sq_fy);
  }

}


/*
  ---------- END OF PICKUP AND RELEASE CODE ---------
*/


const short MOVES_TO_RECENTER = 6;

int main() {

  Engine engine;
  timer bot_move_timer;

  bool isCheckmate = false, isPlayerMove = true;
  short moveCounter = 0;
  short mate_type = 0;

  while(!mate_type)
  {
  if (moveCounter % MOVES_TO_RECENTER == 0)
    zero_xyz();

  short sq_ix = 0, sq_iy = 0, sq_fx = 0, sq_fy = 0, sq_i = 0, sq_f = 0;

  bool is_promote = 0;

  if (isPlayerMove)
  {

    wait_for_move(engine, sq_ix, sq_iy, sq_fx, sq_fy, sq_i, sq_f, isCheckmate);

  check_and_capture(engine, sq_fx, sq_fy);

  engine.move(engine.hash, is_promote, WHITE, engine.squares[sq_i]->shape, sq_i, sq_f);
      
  }
  else
  {
    bot_move_timer.clear();
    short mate_type = 0;
    engine.bot_move(sq_i, sq_f, mate_type);
    resetOutput();
    Brain.Screen.print("Bot Move");
    Brain.Screen.newLine();
    Brain.Screen.print("Time %.1f s", bot_move_timer.time(seconds));


      sq_ix = x_of(sq_i);
      sq_iy = y_of(sq_i);
      sq_fx = x_of(sq_f);
      sq_fy = y_of(sq_f);

  check_and_capture(engine, sq_fx, sq_fy);

  engine.move(engine.hash, is_promote, BLACK, engine.squares[sq_i]->shape, sq_i, sq_f);
  }

  convertToGantry(sq_ix, sq_iy);
  convertToGantry(sq_fx, sq_fy);

  move_to(sq_ix, sq_iy);
  pick_up();
  move_to(sq_fx, sq_fy);
  release();

  isPlayerMove = !isPlayerMove;
  moveCounter++;

  if (is_promote)
  {
    resetOutput();
    Brain.Screen.print("Automatically");
    Brain.Screen.newLine();
    Brain.Screen.print("promoted to");
    Brain.Screen.newLine();
    Brain.Screen.print("queen");

    wait(2,seconds);
  }

}

// Shutdown Procedure

  wait(1, seconds);

  if(mate_type == 1){
  resetOutput();
  Brain.Screen.print("Checkmate");
  Brain.Screen.newLine();
  Brain.Screen.print("Winner:");
  Brain.Screen.newLine();
   if (!isPlayerMove)
    Brain.Screen.print("White");
  else
    Brain.Screen.print("Black");

  }

  else{
    Brain.Screen.print("Stalemate");
  }

  wait(3, seconds);

  Brain.programStop();
  return EXIT_SUCCESS;
}