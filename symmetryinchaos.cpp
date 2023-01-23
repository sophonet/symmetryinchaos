#include <SDL/SDL.h>
#include <stdio.h>
#undef main

#include <algorithm>
#include <cmath>
#include <complex>
#include <random>
#include <vector>

#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#else
#include <cxxopts.hpp>
#include <fstream>
#include <iostream>
#include <string>
#include <tuple>
#endif

#include <nlohmann/json.hpp>
using json = nlohmann::json;

class SymmetryGenerator {
   public:
    SymmetryGenerator(const double &lambda, const double &alpha,
                      const double &beta, const double &gamma,
                      const double &delta,
                      const double &omega, const unsigned int &n, const unsigned int &p)
        : _lambda(lambda),
          _alpha(alpha),
          _beta(beta),
          _gamma(gamma),
          _delta(delta),
          _omega(omega),
          _n(n),
          _p(p) {}

    std::complex<double> iterate(const std::complex<double> &z) {
        double zreal = z.real();
        double zimag = z.imag();
        double znorm = std::norm(z);
        double np_factor = 0.0;

        // Compute additions power according to book if delta is set
        if(_delta != 0.0) {
            auto zabs = sqrt(znorm);
            double zc = 1.0;
            double zd = 0.0;
            double ereal = z.real() / zabs;
            double eimag = z.imag() / zabs;

            for (unsigned int i = 0; i < _n * _p; ++i) {
                double za = ereal * zc - eimag * zd;
                double zb = ereal * zd + eimag * zc;
                zc = za;
                zd = zb;
            }
            np_factor = zc * zabs;
        }

        for (unsigned int i = 0; i < _n - 2; ++i) {
            double za = zreal * z.real() - zimag * z.imag();
            double zb = zimag * z.real() + zreal * z.imag();
            zreal = za;
            zimag = zb;
        }

        const double zn_real = z.real() * zreal - z.imag() * zimag;
        const double p = _lambda + _alpha * znorm + _beta * zn_real + _delta * np_factor;
        return std::complex<double>(
            p * z.real() + _gamma * zreal - _omega * z.imag(),
            p * z.imag() - _gamma * zimag + _omega * z.real());
    }

   private:
    const double _lambda;
    const double _alpha;
    const double _beta;
    const double _gamma;
    const double _delta;
    const double _omega;
    const unsigned int _n;
    const unsigned int _p;
};

class SymmetryDrawer {
   public:
    SymmetryDrawer(SDL_Surface *screen, double extent)
        : _surface(screen),
          _pixels((Uint32 *)_surface->pixels),
          _width(screen->w),
          _height(screen->h),
          _extent(extent),
          _canvas(_width * _height) {
        std::fill(_canvas.begin(), _canvas.end(), 0);
    }

    unsigned short increment(const double &x, const double &y) {
        int px = (y / _extent + 0.5) * _width;
        int py = (-x / _extent + 0.5) * _height;
        if (px >= 0 && px < _width && py >= 0 && py < _height) {
            return ++*(_canvas.begin() + py * _width + px);
        }
        return 0;
    }

    void colorize(const std::vector<std::array<unsigned char, 3>> palette) {
        Uint32 *pix = _pixels;
        auto canvas = _canvas.begin();
        for (; canvas != _canvas.end(); ++pix, ++canvas) {
            unsigned int idx =
                std::min(*canvas, (unsigned short)(palette.size() - 1));
            *pix = SDL_MapRGB(_surface->format, palette[idx][0],
                               palette[idx][1], palette[idx][2]);
        }
    }

    void clear() { SDL_FillRect(_surface, NULL, 0x000000); }

   private:
    SDL_Surface *_surface;
    Uint32 *_pixels;
    unsigned int _width;
    unsigned int _height;
    double _extent;
    std::vector<unsigned short> _canvas;
};

using TPaletteControlPoint = std::array<double, 4>;

std::vector<std::array<unsigned char, 3>> build_palette(
    std::vector<std::array<double, 4>> controlPoints, unsigned int maxval) {
    std::vector<std::array<unsigned char, 3>> palette(maxval);
    auto controlPoint = controlPoints.begin();
    auto color = palette.begin();

    double step = 1.0 / maxval;
    std::array<double, 4> interval_diff;
    std::transform((controlPoint + 1)->begin(), (controlPoint + 1)->end(),
                   controlPoint->begin(), interval_diff.begin(),
                   std::minus<double>());

    double range_norm = 1.0 / interval_diff[0];

    for (double r = 0.0; r < 1.0; r += step, ++color) {
        if (r > (*(controlPoint + 1))[0]) {
            ++controlPoint;
            std::transform((controlPoint + 1)->begin(),
                           (controlPoint + 1)->end(), controlPoint->begin(),
                           interval_diff.begin(), std::minus<double>());
            range_norm = 1.0 / interval_diff[0];
        }
        double inter = (r - (*controlPoint)[0]) * range_norm;
        std::transform(
            interval_diff.begin() + 1, interval_diff.end(),
            controlPoint->begin() + 1, color->begin(),
            [&inter](const double &diff, const double &base) {
                return static_cast<unsigned char>(inter * diff + base);
            });
    }
    return palette;
}

class Runner {
   public:
    Runner(SDL_Surface *screen)
        : _screen(screen),
          _symmetryGenerator(nullptr),
          _symmetryDrawer(nullptr),
          _palette(0),
          _running(false) {}

    void start(
        // Generator
        const double &lambda, const double &alpha, const double &beta,
        const double &gamma, const double &delta, const double &omega, const unsigned int &n, const unsigned int &p,
        // Drawer
        const double &extent,
        // Palette creator
        const std::vector<std::array<double, 4>> &controlPoints,
        const int maxhit, const long int tick_iterations,
        const long int total_iterations) {
        _symmetryGenerator = std::make_unique<SymmetryGenerator>(
            lambda, alpha, beta, gamma, delta, omega, n, p);
        _symmetryDrawer = std::make_unique<SymmetryDrawer>(_screen, extent);
        _palette = build_palette(controlPoints, maxhit);
        _z = {0.001, 0.002};
        // Skip transients
        for (unsigned int i = 0; i < 20; ++i) {
            _z = _symmetryGenerator->iterate(_z);
        }
        _tick_iterations = tick_iterations;
        _total_iterations = total_iterations;
        _running_iterations = 0;
        _running = true;
    }

    void tick() {
        for (unsigned long i = 0; i < _tick_iterations; ++i) {
            _z = _symmetryGenerator->iterate(_z);
            _symmetryDrawer->increment(_z.real(), _z.imag());
        }
        _symmetryDrawer->colorize(_palette);
        SDL_Flip(_screen);
        _running_iterations += _tick_iterations;
        if (_running_iterations > _total_iterations) {
            stop();
        }
    }

    bool running() const { return _running; }

    void stop() { _running = false; };

   public:
    SDL_Surface *_screen;
    std::unique_ptr<SymmetryGenerator> _symmetryGenerator;
    std::unique_ptr<SymmetryDrawer> _symmetryDrawer;
    std::vector<std::array<unsigned char, 3>> _palette;
    std::complex<double> _z;
    unsigned long int _tick_iterations;
    unsigned long int _total_iterations;
    unsigned long int _running_iterations;
    bool _running;
};

// Global runner
static Runner *runner = nullptr;

#ifdef __EMSCRIPTEN__
void emscripten_mainloop() {
    if (SDL_MUSTLOCK(runner->_screen)) SDL_LockSurface(runner->_screen);
    if (runner->running()) {
        runner->tick();
    }
    if (SDL_MUSTLOCK(runner->_screen)) SDL_UnlockSurface(runner->_screen);
}

extern "C" {
void launch(const char *json_parameters) {
    if (SDL_MUSTLOCK(runner->_screen)) SDL_LockSurface(runner->_screen);
    runner->stop();
    json dataset = json::parse(std::string(json_parameters));
    runner->start(dataset["lambda"].get<double>(), dataset["alpha"].get<double>(),
        dataset["beta"].get<double>(), dataset["gamma"].get<double>(),
        dataset["delta"].get<double>(),
        dataset["omega"].get<double>(), dataset["n"].get<unsigned int>(),
        dataset["p"].get<unsigned int>(),
        dataset["extent"].get<double>(),
        dataset["palette"].get<std::vector<std::array<double, 4>>>(), 1200,
        100000, 5E7);
    if (SDL_MUSTLOCK(runner->_screen)) SDL_UnlockSurface(runner->_screen);
}
}

#endif

int main(int argc, char *argv[]) {
    SDL_Init(SDL_INIT_VIDEO);
    SDL_Surface *screen = SDL_SetVideoMode(1000, 1000, 32, SDL_SWSURFACE);
    runner = new Runner(screen);

#ifdef TEST_SDL_LOCK_OPTS
    EM_ASM(
        "SDL.defaults.copyOnLock = false; SDL.defaults.discardOnLock = true; "
        "SDL.defaults.opaqueFrontBuffer = false;");
#endif

#ifdef __EMSCRIPTEN__
    emscripten_set_main_loop(emscripten_mainloop, 0, true);
#else
    cxxopts::Options options("Symmetry in Chaos", "Plots symmetry datasets");
    options.add_options()("d,dataset", "Dataset name",
                          cxxopts::value<std::string>());
    auto result = options.parse(argc, argv);
    if (result.count("dataset") == 0) {
        std::cout << options.help() << std::endl;
        return EXIT_FAILURE;
    }

    std::ifstream f("datasets.json");
    json datasets = json::parse(f);

    json dataset = datasets[result["dataset"].as<std::string>()];

    runner->start(
        dataset["lambda"].get<double>(), dataset["alpha"].get<double>(),
        dataset["beta"].get<double>(), dataset["gamma"].get<double>(),
        dataset["delta"].get<double>(),
        dataset["omega"].get<double>(), dataset["n"].get<unsigned int>(),
        dataset["p"].get<unsigned int>(),
        dataset["extent"].get<double>(),
        dataset["palette"].get<std::vector<std::array<double, 4>>>(), 1200,
        10000000, 8E7);
    if (SDL_MUSTLOCK(screen)) SDL_LockSurface(screen);
    SDL_Event event;

    while (runner->running()) {
        runner->tick();
        SDL_PollEvent(&event);
        if (event.type == SDL_QUIT) {
            runner->stop();
        }
    }
    std::cout << "Done" << std::endl;
    while (event.type != SDL_QUIT) {
        SDL_WaitEvent(&event);
    }
    if (SDL_MUSTLOCK(screen)) SDL_UnlockSurface(screen);
    SDL_Quit();
#endif
    return EXIT_SUCCESS;
}
