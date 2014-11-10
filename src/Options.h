#ifndef OPTIONS_H
#define OPTIONS_H

#include <string>

class Options
{
public:
	static Options * Instance()
	{
		// C++ 不保证静态局部变量线程安全
		static Options options;
		return &options;
	}
public:
	bool ParseCommandLine(int argc, char ** argv);
	bool ParseConfigFile(char * filename);
	void Usage();
	void Show();

public:
	bool UDPMode;
	bool ServerMode;
	// 对客户端 为 服务器地址
	// 对服务器 为 监听地址
	std::string ServerAddress;
	std::string ServerPort;
	std::string LoginID;
	
	std::string ControlAddress;
	std::string ControlPort;

	// 可执行文件名
	std::string ExecName;
	
private:
	Options();
	Options(const Options &);
	Options & operator = (const Options &);
};

#endif // OPTIONS_H
