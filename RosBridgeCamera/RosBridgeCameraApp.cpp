#include <iostream>
#include <cstdarg>

#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>

#include "bson/bsonobjbuilder.h"
#include "bson/json.h"

#include "socketcpp/Socket.h"


class RosBridgeCameraApp
{
    SocketClient *socket_;
	cv::VideoCapture webcam_;

	std::string image_topic_name_;
	std::string image_topic_type_;

	std::string text_topic_name_;
	std::string text_topic_type_;

    cv::Mat GetCameraImage()
    {
		cv::Mat image;
		webcam_ >> image;
		if (image.empty())
		{
			std::cout << "Empty image" << std::endl;
		}
		cv::imshow("image", image);
		cv::waitKey(50);

		return image.clone();
    }

    void AdvertiseTopic(std::string topic_name, std::string topic_type)
    {
        _bson::bsonobjbuilder cmd;

        cmd.append("op", "advertise");
        cmd.append("topic", topic_name);
        cmd.append("type", topic_type);

        _bson::bsonobj o = cmd.obj();
		if (NULL != socket_)
			socket_->SendBinary(o.objdata(), o.objsize());
        
    }

    void UnadvertiseTopic(std::string topic_name)
    {
        _bson::bsonobjbuilder cmd;

        cmd.append("op", "unadvertise");
        cmd.append("topic", topic_name);

        _bson::bsonobj o = cmd.obj();
		if (NULL != socket_)
			socket_->SendBinary(o.objdata(), o.objsize());
    }

    void PublishString(std::string topic_name, std::string message_str)
    {
        _bson::bsonobjbuilder cmd, msg;
        cmd.append("op", "publish");
        cmd.append("topic", topic_name);
        cmd.append("msg", msg.append("data", message_str).obj());
 
        _bson::bsonobj o = cmd.obj();
		if (NULL != socket_)
			socket_->SendBinary(o.objdata(), o.objsize());

		std::cout << message_str << std::endl;
    }

    void PublishImage(std::string topic_name, cv::Mat message_mat)
    {
        _bson::bsonobjbuilder cmd, msg, header;

        std::stringstream header_str("{seq:0,frame_id:\"webcam\"}");

        msg.append("header", _bson::fromjson(header_str, header));
		std::string encoding;
		if (message_mat.elemSize() == 1)
			encoding = "mono8";
		else
			encoding = "bgr8";
        msg.append("encoding", encoding);
        msg.append("is_bigendian", 0);
        msg.append("height", message_mat.rows);
        msg.append("width", message_mat.cols);
        msg.append("step", (int) (message_mat.cols * message_mat.elemSize())); //width

        msg.appendBinData("data",
            message_mat.rows*message_mat.cols*message_mat.elemSize(),
            _bson::BinDataType::BinDataGeneral,
            (void*)message_mat.data);

        cmd.append("op", "publish");
        cmd.append("topic", topic_name);
        cmd.append("msg", msg.obj());

        _bson::bsonobj o = cmd.obj();
		if (NULL != socket_)
			socket_->SendBinary(o.objdata(), o.objsize());
    }

	void InitializeCamera()
	{
		std::string text_message;
		webcam_.open(0);
		if (!webcam_.isOpened())
		{
			text_message = "Error while initializing camera";
			PublishString(text_topic_name_, text_message);
		}
		else
		{
			text_message = "Camera Initialized corretly";
			PublishString(text_topic_name_, text_message);
		}
	}

public:
	RosBridgeCameraApp()
	{
		image_topic_name_ = "/webcam/image";
		image_topic_type_ = "sensor_msgs/Image";

		text_topic_name_ = "/webcam/image_status";
		text_topic_type_ = "std_msgs/String";
	}

	void SetSocket(SocketClient& socket)
	{
		socket_ = &socket;
	}

	void Stop()
	{
		webcam_.release();
		
		PublishString(text_topic_name_, "Camera Stopped");
	}

    void Run()
    {
		if (NULL == socket_)
		{
			std::cout << "ERROR: INVALID SOCKET " << std::endl;
			return;
		}
		AdvertiseTopic(image_topic_name_, image_topic_type_);
		AdvertiseTopic(text_topic_name_, text_topic_type_);

		InitializeCamera();

		PublishString(text_topic_name_, "Advertising both topics... Waiting 2 secs");

        Sleep(2000);
        
		PublishString(text_topic_name_, "Publishing the image 20 times");
        for (int i = 0; i < 300; i++)
        {
			cv::Mat image = GetCameraImage();
            PublishImage(image_topic_name_, image);
            PublishString(text_topic_name_, "Image Sent, waiting 50ms");
        }

		PublishString(text_topic_name_, "Unadvertising... Finishig in 2 secs");
        UnadvertiseTopic(image_topic_name_);
		UnadvertiseTopic(text_topic_name_);

		Stop();

        Sleep(2000);
    }
};

int main(int argc, char* argv[])
{
    std::string client_ip = "127.0.0.1";
    int client_port = 9090;

	if (argc > 1)
	{
		client_ip = argv[1];
		if (argc > 2)
		{
			std::istringstream ss(argv[2]);
			if (!(ss >> client_port))
				std::cerr << "Invalid number " << argv[2] << std::endl;
		}
	}

	std::cout << "IP:" << client_ip << " port:" << client_port;
try_again:
	RosBridgeCameraApp client;
	
	try {
		SocketClient socket(client_ip, client_port);
	
		std::cout << "Connected to " << client_ip << " at port " << client_port << std::endl;
		
		client.SetSocket(socket);

		client.Run();

		socket.Close();
		std::cout << "Closed connection" << std::endl;
	}
	catch (...)
	{
		client.Stop();
		std::cout << "COULD NOT connect: waiting 5s to retry" << std::endl;
		Sleep(5000);
		goto try_again;
	}
    return 0;
}