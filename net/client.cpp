#include <vector>
#include <iostream>
#include <string>
//#include <sstream>
#include <SFML/Network.hpp>
#include <SFML/System.hpp>
#include "include/Ball.h"
#include "include/Hole.h"
#include "include/Cane.h"
#include "include/Text.h"

sf::Vector2f rp[2];
bool playable = true;
bool lost;
bool turn = false;
bool received = false;
sf::TcpSocket socket;

void netloop()
{
    while(1)
    {
        int id;
        sf::Packet packet;
        socket.receive(packet);
        packet >> id;

        switch(id)
        {
            case 0:
            float x, y;
            int n;
            packet >> x >> y >> n;
            rp[n] = sf::Vector2f(x,y);
            break;

            case 1:
            packet >> turn;
            break;

            case 2:
            packet >> lost;
            playable = false;
            break;
        }
    }
}

int main()
{
    srand(time(NULL));
    sf::Clock cl;
    float dt = 0;

	//int score = 0;

	//std::ostringstream sScore;

	sf::RectangleShape rectangle;
	rectangle.setSize(sf::Vector2f(400, 300));
	rectangle.setFillColor(sf::Color(60,140,70));
	rectangle.setPosition(400,300);

	sf::RectangleShape rectangle2;
	rectangle2.setSize(sf::Vector2f(1600, 50));
	rectangle2.setFillColor(sf::Color(60, 140, 70));
	rectangle2.setPosition(0, 550);

	//sf::Text lblScore;
	//lblScore.setCharacterSize(30);
	//lblScore.setPosition(10, 10);

	//sf::Font arial;
	//arial.loadFromFile("arial.ttf");

	//lblScore.setFont(arial);
	//sScore << "Score: " << score;


    std::cout << "Enter IP address:" << std::endl;
    std::string strip;
    std::cin >> strip;
    sf::IpAddress ip(strip);
    socket.connect(ip,7777);
    std::cout << "Connection established" << std::endl;

    std::vector<Ball> balls(2);
    std::vector<std::vector<bool>> validcollisions(2,std::vector<bool>(1000,true));
    balls[0].setPosition(sf::Vector2f(100,535));
    balls[1].setPosition(sf::Vector2f(1130,535));

	balls[0].setFillColor(sf::Color::Black);
	balls[1].setFillColor(sf::Color::White);

    std::vector<Hole> holes(1);
    holes[0].setPosition(sf::Vector2f(600,300));

    Cane cane;
    Text endtext;

    bool drawcane = false;
    bool moveable = true;

    sf::ContextSettings settings;
    settings.antialiasingLevel = 4;

    sf::RenderWindow window(sf::VideoMode(1200,600), "Golf - Client", sf::Style::Default, settings);
    window.setFramerateLimit(60);

    sf::Thread netthread(&netloop);
    netthread.launch();

    while(window.isOpen())
    {
        window.clear(sf::Color(60,125,140));//sky colour = blue

		window.draw(rectangle);
		window.draw(rectangle2);

        for(auto h : holes)
        window.draw(h);
        for(auto b : balls)
        window.draw(b);


        if(cane.drawcane)
        {
            window.draw(cane.aim);
        }
        if(!playable)
        window.draw(endtext);

        sf::Vector2f mpos = (sf::Vector2f)sf::Mouse::getPosition(window);
        sf::Vector2f cpos = balls[1].getPosition();//pull animation on white ball
        cane.update(mpos,cpos);

        moveable = true;
        for(int i = 0; i < balls.size(); i++)
        {
            if(!balls[i].isHidden())
            {
                sf::Vector2f oldpos = balls[i].getPosition();
                if(oldpos != rp[i])
                moveable = false;
            }
            balls[i].setPosition(rp[i]);
        }

        sf::Event event;
        while(window.pollEvent(event))
        {
            if(event.type == sf::Event::Closed)
            {
                window.close();
                netthread.terminate();
            }
            if(event.type == sf::Event::MouseButtonPressed and sqrt(sfm::len2(mpos-cpos)) <= 15 and moveable and turn)
            {
                cane.drawcane = true;
            }
            if(event.type == sf::Event::MouseButtonReleased and cane.drawcane)
            {
                sf::Vector2f releasevel = sf::Vector2f(-5*cane.vec.x,-5*cane.vec.y);
                cane.drawcane = false;

                sf::Packet packet;
                float x = releasevel.x;
                float y = releasevel.y;
                packet << x << y;
                socket.send(packet);
            }
        }

        if(!playable)
        {
            endtext.set((std::string)(lost?"YOU LOST":"YOU WON"));
            endtext.center(1200,600);
        }

        window.display();
        dt = cl.restart().asSeconds();
    }

    return 0;
}
