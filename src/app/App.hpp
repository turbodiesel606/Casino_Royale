#ifndef JOBTRACKER_SRC_APP_APP_HPP
#define JOBTRACKER_SRC_APP_APP_HPP

class App final
{
public:
    static int start(int argc, char* argv[]);

private:
    App() = delete;
};

#endif // JOBTRACKER_SRC_APP_APP_HPP
