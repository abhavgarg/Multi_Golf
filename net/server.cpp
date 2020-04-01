#include <vector>
#include <iostream>
#include <string>

#include <SFML/Network.hpp>
#include <SFML/System.hpp>
#include "include/Ball.h"
#include "include/Hole.h"
#include "include/Cane.h"
#include "include/Text.h"

sf::Vector2f rv;
bool playable = true;
bool received = false;
sf::TcpSocket socket;

void netloop()
{
    while(1)
    {
        float x, y;
        sf::Packet packet;
        socket.receive(packet);
        packet >> x >> y;
        rv = sf::Vector2f(x,y);
        received = true;
    }
}

int main()
{
	srand(time(NULL));
	sf::Clock cl;
	float dt = 0;




	sf::RectangleShape rectangle;//platform
	rectangle.setSize(sf::Vector2f(400, 300));
	rectangle.setFillColor(sf::Color(60, 140, 70));
	rectangle.setPosition(400, 300);
	sf::Text lblScore;


	sf::RectangleShape rectangle2;//ground
	rectangle2.setSize(sf::Vector2f(1600, 50));
	rectangle2.setFillColor(sf::Color(60, 140, 70));
	rectangle2.setPosition(0, 550);

	std::cout << "Waiting for connection" << std::endl;
	sf::TcpListener listener;
	listener.listen(7777);
	listener.accept(socket);
	std::cout << "Connection established" << std::endl;

	std::vector<Ball> balls(2);//initial positions of both balls
	std::vector<std::vector<bool>> validcollisions(2, std::vector<bool>(1000, true));
	balls[0].setPosition(sf::Vector2f(100, 535));
	balls[1].setPosition(sf::Vector2f(1100, 535));
	balls[0].setFillColor(sf::Color::Black);
	balls[1].setFillColor(sf::Color::White);


	std::vector<Hole> holes(1);
	holes[0].setPosition(sf::Vector2f(600, 300));//hole is in middle of scene

	Cane cane;
	Text endtext;

	bool drawcane = false;
	bool turn = true;
	bool moveable = true;
	bool scored = false;
	bool changeable = false;
	bool pickedcolor = false;
	bool isblue = true;
	bool lost;

	int bhole[2];
	int points[2] = { 0,0 };

	sf::ContextSettings settings;
	settings.antialiasingLevel = 4;

	sf::RenderWindow window(sf::VideoMode(1200, 600), "Golf - Server", sf::Style::Default, settings);
	window.setFramerateLimit(60);

	sf::Thread netthread(&netloop);
	netthread.launch();

	while (window.isOpen())
	{
		window.clear(sf::Color(60, 125, 140));//sky colour = blue

		window.draw(rectangle);
		window.draw(rectangle2);

		for (auto h : holes)
			window.draw(h);
		for (auto b : balls)
			window.draw(b);
		if (cane.drawcane)
		{
			window.draw(cane.aim);
		}
		if (!playable)
			window.draw(endtext);

		sf::Vector2f mpos = (sf::Vector2f)sf::Mouse::getPosition(window);
		sf::Vector2f cpos = balls[0].getPosition();
		cane.update(mpos, cpos);

		for (int i = 0; i < balls.size(); i++)
		{
			if (!balls[i].isHidden())
				balls[i].move(dt);
			for (int j = 0; j < balls.size(); j++)
			{
				if (j == i)
					continue;
				if (balls[1].collide(balls[0].getPosition()))
				{
					if (validcollisions[i][j])
					{
						sf::Vector2f v01 = balls[0].getVelocity();
						sf::Vector2f v02 = balls[1].getVelocity();
						sf::Vector2f x01 = balls[0].getPosition();
						sf::Vector2f x02 = balls[1].getPosition();
						balls[0].setVelocity(v01 - (sfm::dot(v01 - v02, x01 - x02)) / (sfm::len2(x01 - x02)) * (x01 - x02));
						balls[1].setVelocity(v02 - (sfm::dot(v02 - v01, x02 - x01)) / (sfm::len2(x01 - x02)) * (x02 - x01));
						validcollisions[0][1] = false;
						validcollisions[1][0] = false;
					}
				}
				else
				{
					validcollisions[0][1] = true;
					validcollisions[1][0] = true;
				}
			}
			sf::Packet packet;

			float x = balls[i].getPosition().x;
			float y = balls[i].getPosition().y;

			int id = 0;
			packet << id << x << y << i;

			socket.send(packet);//send collision packets in id=0

			if (holes[0].enter(balls[i].getPosition()))
			{
				balls[i].hide();

				if (i == 0)
				{
					//if((turn and isblue) or (!turn and !isblue))
					scored = true;
					points[!isblue]++;

					if (i == 1)
					{

						scored = true;
						points[isblue]++;

					}
				}
			}
			
		}
		moveable = true;
		for (auto b : balls)
		{
			if (sfm::len2(b.getVelocity()) != 0 and !b.isHidden())
				moveable = false;
		}

		if (moveable and !scored and changeable)
		{
			turn = !turn;
			changeable = false;
			sf::Packet tpacket;
			int id = 1;
			tpacket << id << !turn;
			socket.send(tpacket); //id=1 used to send turn
		}


		sf::Event event;
		while (window.pollEvent(event))
		{
			if (event.type == sf::Event::Closed)
			{
				window.close();
				netthread.terminate();
			}
			if (event.type == sf::Event::MouseButtonPressed and sqrt(sfm::len2(mpos - cpos)) <= balls[0].getRadius() and moveable and turn)
			{
				cane.drawcane = true;
			}
			if (event.type == sf::Event::MouseButtonReleased and cane.drawcane)
			{
				balls[0].setVelocity(sf::Vector2f(-5 * cane.vec.x, -5 * cane.vec.y));
				cane.drawcane = false;
				scored = false;
				changeable = true;
				balls[0].setPosition(sf::Vector2f(100, 535));
				balls[1].setPosition(sf::Vector2f(1100, 535));
				balls[0].setFillColor(sf::Color::Black);
				balls[1].setFillColor(sf::Color::White);
			}
		}

		if (received)
		{
			balls[1].setVelocity(rv);
			received = false;
			scored = false;
			changeable = true;
			balls[0].setPosition(sf::Vector2f(100, 535));
			balls[1].setPosition(sf::Vector2f(1100, 535));
			balls[0].setFillColor(sf::Color::Black);
			balls[1].setFillColor(sf::Color::White);
		}

		window.display();
		dt = cl.restart().asSeconds();

	}
	return 0;
	
}
