#include "orbit.h"
#include <iostream>

vec calculateGravityForce(vec distance, double shipMass)
{
    double distanceScalar = distance.getScalar();
    if (distanceScalar == 0) {
        vec res = {0, 0, 0};
        return res ;
    }
    double a = G * EarthMass * shipMass / pow(distanceScalar, 3);
    vec res = distance.multiplyWithDouble(a);
    return res;
}

double temperature(double height) //calculates the temperature at a certain height (K)
{
    if (height <= 0)
    {
        throw invalid_argument("Height is less than the radius of the Earth");
    }
    double temperature = ZeroCelsius;
    if (height > 145) //at each height there's an equation of temperature calculation
    {
        return 0.0;
    }
    if (height > 94)
    {
        temperature += 140 * (height - 94) / 51 - 90;
    }
    if (height > 84)
    {
        temperature -= 90;
    }
    if (height > 54)
    {
        temperature += -3 * height + 162;
    }
    if (height > 47)
    {
        return temperature;
    }
    if (height > 20)
    {
        temperature += (20 * height - 940) / 9;
    }
    if (height > 10)
    {
        temperature -= 60;
    }
    else
    {
        temperature += -8 * height + 20;
    }
    return temperature;
}

double airDensity(double height) //calculates the air density at a certain height
{
    height -= EarthRadius;
    double T = temperature(height); //temperature at a certain height
    if (T <= 0.0)
    {
        return 0.0;
    }
    else
    {
        double p = 101325, //normal atmospheric pressure at sea level (Pa)
        g = G * EarthMass / pow(height, 2), //acceleration due to gravity
                                            //on the Earth's surface (m/s^2)
        R = 8.31447, //universal gas constant (J / (mol * K))
        M = 0.0289644, //the molar mass of dry air (kg / mol)
        P = p * exp(-M * g * height * 1000 / (R * T)), //pressure at the certain height (Pa)
        density = P * M / (R * T); //air density at the certain height (kg / m^3)
        return density * pow(10.0, 9); //transfer to kg / km^3
    }
}

vec calculateAerodynamicForce(vec speed, double square, double height)
{
    double v = speed.getScalar();
    double p = airDensity(height);
    vec result = speed.multiplyWithDouble(-p * v * square / 2);
    return result;
}

vec calculateTractiveForce(double massLevel, double specificImpulse, vec speed)
{
    double v = speed.getScalar();
    if (v == 0) {
        vec result = {0, 0, 0};
        return result;
    }
    vec result = speed.multiplyWithDouble(massLevel * specificImpulse / v);
    return result;
}

vec calculateAngularVelocity(vec gravityForce, vec aerodynamicForce,
                             vec tractiveForce, Rotation moment, double quantSizeOfSec,
                             double length, vec previousAngularVelocity)
{
    double x = moment.rotationAroundX, //moment of inertia
           y = moment.rotationAroundY,
           z = moment.rotationAroundZ;
    vec distance = {length / 2, length / 2, length / 2}; //distance to the center of rotation
    vec resultForce;
    resultForce.x = gravityForce.x + aerodynamicForce.x + tractiveForce.x;
    resultForce.y = gravityForce.y + aerodynamicForce.y + tractiveForce.y;
    resultForce.z = gravityForce.z + aerodynamicForce.z + tractiveForce.z;
    vec momentForce = distance.multiply(resultForce); //moment of force
    if (x != 0)
    {
        x = momentForce.x * quantSizeOfSec / x;
    }
    if (y != 0)
    {
        y = momentForce.y * quantSizeOfSec / y;
    }
    if (z != 0)
    {
        z = momentForce.z * quantSizeOfSec / z;
    }
    vec currentAngularVelocity;
    currentAngularVelocity.x = previousAngularVelocity.x + x;
    currentAngularVelocity.y = previousAngularVelocity.y + y;
    currentAngularVelocity.z = previousAngularVelocity.z + z;
    return currentAngularVelocity;
}

double aerodynamicHeating(double temperature, vec speed)
{
    double heating = temperature + pow(speed.getScalar(), 2) / 2; // (K)
    return heating;
}

vec speed(vec previousSpeed, vec position, vec orientation, double fuelConsumption,
          double mShip, double mFuel, Rotation moment, double specificImpulse,
          double size, double quantSizeOfSec, double maxOverload, double maxHeating)
//calculates a speed at each time interval
{
    double mTotal = mShip + mFuel;
    double H = position.getScalar();
    double scSpeedFirst = previousSpeed.getScalar();
    double S = size * size;
    vec exit = {0, 0, 0};
    if (mTotal != 0) {
        if (quantSizeOfSec > 0.0)
        {
            double v1 = airDensity(H) * scSpeedFirst * S / (2.0 * mTotal),
            v2 = 1 / quantSizeOfSec  - v1,
            v3 = calculateTractiveForce(fuelConsumption, specificImpulse,
                                        previousSpeed).getScalar() / mTotal,
            v4 = G * EarthMass / pow(H, 3);
            
            vec t1 = previousSpeed.multiplyWithDouble(v1),
            t2 = previousSpeed.multiplyWithDouble(v2),
            t3 = orientation.multiplyWithDouble(v3),
            t4 = position.multiplyWithDouble(v4);
            exit.x = (t2.x + t3.x - t4.x) * quantSizeOfSec;
            exit.y = (t2.y + t3.y - t4.y) * quantSizeOfSec;
            exit.z = (t2.z + t3.z - t4.z) * quantSizeOfSec;
            
            vec overload = {0, 0, 0};
            overload.x = - t1.x + t3.x - t4.x;
            overload.y = - t1.y + t3.y - t4.y;
            overload.z = - t1.z + t3.z - t4.z;
            double g = G * EarthMass / pow(H, 2),
            over = overload.getScalar() / g;

            if (over > maxOverload)
            {
                throw invalid_argument("Overload");
            }
        }
        else
        {
            return previousSpeed;
        }
    }
    
    if (H <= 6523.1)
    {
        double a = aerodynamicHeating(temperature(position.getScalar()), exit);
        if (a > maxHeating)
        {
            throw invalid_argument("Overheating");
        }
    }
    return exit;
}

vector <ReturnValues> computeFlightPlan(ShipPosition initialPosition,
                                        ShipParams shipParams, Quants quants)
{
    double S = pow(shipParams.shipEdgeLength, 2),
    height = initialPosition.position.getScalar(),
    fuel = shipParams.fuelMass,
    m = fuel + shipParams.shipMass;
    double level;
    vec currentSpeed = initialPosition.speedFirst;
    Rotation moment = initialPosition.moment;
    vec currentOrientation = initialPosition.orientation;
    vec previousAngularVelocity = {0, 0, 0};
    vec currentPosition = initialPosition.position;
    
    vector<ReturnValues> calculatedFlightPlan(quants.numberOfQuants);
    int i = 0,
    j = 0;
    double count = shipParams.flightPlan[0].delayTime;
    for (i = 0; i < quants.numberOfQuants && height > EarthRadius; i++)
    {
        level = shipParams.flightPlan[j].impulseValue;
        moment = shipParams.flightPlan[j].rotateValue;
        vec gravityForce = calculateGravityForce(currentPosition, m);
        vec aerodynamicForce = calculateAerodynamicForce(currentSpeed, S, height);
        vec tractiveForce = calculateTractiveForce(level, shipParams.impulsePerFuel, currentSpeed);
        previousAngularVelocity = calculateAngularVelocity(
                                    gravityForce, aerodynamicForce, tractiveForce,
                                    moment, quants.quantSizeOfSec, shipParams.shipEdgeLength,
                                    previousAngularVelocity);
        currentOrientation = currentOrientation.rotate(previousAngularVelocity.createQuaternion());
        if (level * quants.quantSizeOfSec > fuel) //if we don't have enough fuel
        {
            double time = fuel / level, //time of flight with fuel
                timeOut = quants.quantSizeOfSec - time; //time of flight without fuel
            currentSpeed = speed(currentSpeed, currentPosition, currentOrientation,
                                 level, shipParams.shipMass, fuel, moment,
                                 shipParams.impulsePerFuel, shipParams.shipEdgeLength,
                                 time, shipParams.maxOverload, shipParams.maxHeating);
            calculatedFlightPlan[i].speed = currentSpeed;
            m -= fuel;
            fuel = 0.0;
            currentPosition = {currentPosition.x + currentSpeed.x * time,
                               currentPosition.y + currentSpeed.y * time,
                               currentPosition.z + currentSpeed.z * time};
            currentSpeed = speed(currentSpeed, currentPosition, currentOrientation, 0.0,
                       shipParams.shipMass, fuel, moment,
                       shipParams.impulsePerFuel, shipParams.shipEdgeLength,
                       timeOut, shipParams.maxOverload, shipParams.maxHeating);
        }
        else //if we have enough fuel
        {
            currentSpeed = speed(currentSpeed, currentPosition, currentOrientation,
                                 level, shipParams.shipMass, fuel, moment,
                                 shipParams.impulsePerFuel, shipParams.shipEdgeLength,
                                 quants.quantSizeOfSec, shipParams.maxOverload,
                                 shipParams.maxHeating);
            calculatedFlightPlan[i].speed = currentSpeed;
            fuel -= level * quants.quantSizeOfSec;
            m -= level * quants.quantSizeOfSec;
        }
        currentPosition = {currentPosition.x + currentSpeed.x * quants.quantSizeOfSec,
                           currentPosition.y + currentSpeed.y * quants.quantSizeOfSec,
                           currentPosition.z + currentSpeed.z * quants.quantSizeOfSec};
        height = currentPosition.getScalar();
        cout << currentPosition.getScalar() << " "<<EarthRadius<<"\n\n";
        if (height < EarthRadius + 0.1 && height >= EarthRadius) //too small value to calculate the exact
        {
            height = EarthRadius;
            calculatedFlightPlan[i].position = currentPosition;
            break;
        }
        if (height < EarthRadius) //we are below the level of the ground surface
                                  //so we need to calculate an exact position of landing
        {
            vec way = {currentSpeed.x * quants.quantSizeOfSec,
                       currentSpeed.y * quants.quantSizeOfSec,
                       currentSpeed.z * quants.quantSizeOfSec};
            double cosA = (pow(way.getScalar(), 2) + pow(height, 2) -
                           pow(currentPosition.getScalar(), 2)) / (2 * way.getScalar() * height);
            double extraWay = height * cosA - sqrt(pow(height, 2) * pow(cosA, 2) +
                                              pow(EarthRadius, 2) - pow(height, 2));
            double landingTime = extraWay / currentSpeed.getScalar();
            currentPosition.x = currentPosition.x + currentSpeed.x * landingTime;
            currentPosition.y = currentPosition.y + currentSpeed.y * landingTime;
            currentPosition.z = currentPosition.z + currentSpeed.z * landingTime;
            height = EarthRadius;
            calculatedFlightPlan[i].position = currentPosition;
            break;
        }
        
        height = currentPosition.getScalar();
        calculatedFlightPlan[i].position = currentPosition;
        if (count <= 0) //delay time is over, so we need to take the next pack of commands
        {
            j++;
            count = shipParams.flightPlan[j].delayTime;
        }
        else { count -= quants.quantSizeOfSec; }
    }
    if (height <= EarthRadius) //we have landed, so position & speed will not be changed
    {
        int j;
        for (j = i; j < quants.numberOfQuants; j++)
        {
            
            calculatedFlightPlan[j].position = calculatedFlightPlan[i].position;
        }
    }
    return calculatedFlightPlan;
}