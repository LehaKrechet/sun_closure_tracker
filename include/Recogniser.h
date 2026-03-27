class ARecogniser{
    public:
        virtual void recognize() = 0;
};

class FooSunRecognizer : public ARecogniser{
    private:
        float position;
        float speed;
    public:
        void recognize();
};

class FooCloudRecognizer : public ARecogniser{
    private:
        float position;
        float speed;
    public:
        void recognize();
};