// =============================================================================
// SELECTOR DE PERSONAJES - SDL2
// Todo el programa en un solo archivo, organizado por secciones:
//   1. Includes y constantes
//   2. Estructuras de datos
//   3. Personajes (archivo + vector)
//   4. Audio
//   5. Graficos (UI)
//   6. Game loop (input / update / render)
//   7. main
// =============================================================================

#include <SDL.h>
#include <SDL_image.h>
#include <SDL_mixer.h>
#include <SDL_ttf.h>

#include <fstream>
#include <string>
#include <vector>


// =============================================================================
// 1. CONSTANTES
// =============================================================================

const int WINDOW_WIDTH = 1280;   // maximo permitido: 1920x1080
const int WINDOW_HEIGHT = 720;
const char* WINDOW_TITLE = "Character Select";

const char* CHARACTERS_FILE = "characters.txt";
const char* FONT_PATH = "assets/fonts/arial.ttf";
const char* INDICATOR_PATH = "assets/images/indicator.png";
const char* SOUND_MOVE_PATH = "assets/sounds/move.wav";
const char* SOUND_REMOVE_PATH = "assets/sounds/remove.wav";

const int FONT_SIZE_TITLE = 48;
const int FONT_SIZE_ITEM = 32;
const int FONT_SIZE_LEGEND = 20;

const int LIST_START_X = 120;
const int LIST_START_Y = 140;
const int LIST_ITEM_HEIGHT = 44;
const int INDICATOR_SIZE = 28;

const SDL_Color COLOR_BACKGROUND = { 64, 131, 200, 255 };
const SDL_Color COLOR_TEXT = { 255, 255, 255, 255 };


// =============================================================================
// 2. ESTRUCTURAS DE DATOS
// =============================================================================

// Datos de un luchador.
struct Character
{
    std::string name;

    // TODO (extras): atributos del luchador
    // int health;
    // int strength;
    // SDL_Texture* portrait = nullptr;   // imagen a la derecha de la lista
    // Mix_Chunk*   voice    = nullptr;   // sonido caracteristico
};

// Recursos graficos y de audio (se cargan una vez al iniciar).
struct Resources
{
    SDL_Window* window = nullptr;
    SDL_Renderer* renderer = nullptr;
    TTF_Font* fontTitle = nullptr;
    TTF_Font* fontItem = nullptr;
    TTF_Font* fontLegend = nullptr;
    SDL_Texture* indicator = nullptr;
    Mix_Chunk* soundMove = nullptr;
    Mix_Chunk* soundRemove = nullptr;

    // TODO (extras): SDL_Texture* background = nullptr;
    // TODO (extras): Mix_Music*   music      = nullptr;
};

// Estado de la aplicacion.
struct AppState
{
    std::vector<Character> characters;   // lista en memoria
    int  selectedIndex = 0;              // posicion del indicador
    bool running = true;                 // false => sale del game loop
};


// =============================================================================
// 3. PERSONAJES
// =============================================================================

// Lee el archivo (un nombre por linea) y llena el vector.
// Devuelve false si no pudo abrir el archivo.
bool loadCharacters(const std::string& path, std::vector<Character>& characters)
{
    std::ifstream file(path);
    if (!file.is_open())
    {
        return false;
    }

    characters.clear();

    std::string line;
    while (std::getline(file, line))
    {
        // Elimina el '\r' sobrante de archivos con saltos de linea de Windows.
        if (!line.empty() && line.back() == '\r')
        {
            line.pop_back();
        }

        // Ignora lineas vacias.
        if (line.empty())
        {
            continue;
        }

        Character character;
        character.name = line;

        // TODO (extras): cargar aca los assets/atributos propios del personaje.

        characters.push_back(character);
    }

    return true;
}

// Elimina el personaje en 'index' del vector. Devuelve true si lo elimino.
bool removeCharacter(std::vector<Character>& characters, int index)
{
    if (index < 0 || index >= static_cast<int>(characters.size()))
    {
        return false;
    }

    characters.erase(characters.begin() + index);
    return true;
}


// =============================================================================
// 4. AUDIO
// =============================================================================

bool initAudio(Resources& res)
{
    if (Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 2, 2048) < 0)
    {
        SDL_Log("Mix_OpenAudio error: %s", Mix_GetError());
        return false;
    }

    res.soundMove = Mix_LoadWAV(SOUND_MOVE_PATH);
    res.soundRemove = Mix_LoadWAV(SOUND_REMOVE_PATH);

    if (res.soundMove == nullptr || res.soundRemove == nullptr)
    {
        SDL_Log("No se pudieron cargar los sonidos: %s", Mix_GetError());
        return false;
    }

    // TODO (extras): cargar y reproducir musica de fondo.

    return true;
}

void playSound(Mix_Chunk* sound)
{
    if (sound != nullptr)
    {
        Mix_PlayChannel(-1, sound, 0);   // -1 = primer canal libre, 0 = sin repetir
    }
}


// =============================================================================
// 5. GRAFICOS (UI)
// =============================================================================

bool initGraphics(Resources& res)
{
    res.window = SDL_CreateWindow(WINDOW_TITLE,
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        WINDOW_WIDTH, WINDOW_HEIGHT, SDL_WINDOW_SHOWN);
    if (res.window == nullptr)
    {
        SDL_Log("SDL_CreateWindow error: %s", SDL_GetError());
        return false;
    }

    res.renderer = SDL_CreateRenderer(res.window, -1,
        SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    if (res.renderer == nullptr)
    {
        SDL_Log("SDL_CreateRenderer error: %s", SDL_GetError());
        return false;
    }

    if (TTF_Init() < 0)
    {
        SDL_Log("TTF_Init error: %s", TTF_GetError());
        return false;
    }

    res.fontTitle = TTF_OpenFont(FONT_PATH, FONT_SIZE_TITLE);
    res.fontItem = TTF_OpenFont(FONT_PATH, FONT_SIZE_ITEM);
    res.fontLegend = TTF_OpenFont(FONT_PATH, FONT_SIZE_LEGEND);
    if (res.fontTitle == nullptr || res.fontItem == nullptr || res.fontLegend == nullptr)
    {
        SDL_Log("TTF_OpenFont error: %s", TTF_GetError());
        return false;
    }

    if ((IMG_Init(IMG_INIT_PNG) & IMG_INIT_PNG) == 0)
    {
        SDL_Log("IMG_Init error: %s", IMG_GetError());
        return false;
    }

    res.indicator = IMG_LoadTexture(res.renderer, INDICATOR_PATH);
    if (res.indicator == nullptr)
    {
        SDL_Log("No se pudo cargar el indicador: %s", IMG_GetError());
        return false;
    }

    // TODO (extras): cargar imagen de fondo.

    return true;
}

// Dibuja un texto en pantalla (crea una textura temporal con el texto).
void drawText(SDL_Renderer* renderer, TTF_Font* font, const std::string& text,
    int x, int y, SDL_Color color)
{
    if (text.empty())
    {
        return;
    }

    SDL_Surface* surface = TTF_RenderUTF8_Blended(font, text.c_str(), color);
    if (surface == nullptr)
    {
        return;
    }

    SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, surface);
    SDL_Rect dest = { x, y, surface->w, surface->h };
    SDL_FreeSurface(surface);

    if (texture != nullptr)
    {
        SDL_RenderCopy(renderer, texture, nullptr, &dest);
        SDL_DestroyTexture(texture);
    }
}

void drawTitle(Resources& res)
{
    drawText(res.renderer, res.fontTitle, "LIST OF FIGHTERS", LIST_START_X, 50, COLOR_TEXT);
}

void drawCharacterList(Resources& res, const std::vector<Character>& characters, int selectedIndex)
{
    for (int i = 0; i < static_cast<int>(characters.size()); ++i)
    {
        int y = LIST_START_Y + i * LIST_ITEM_HEIGHT;

        drawText(res.renderer, res.fontItem, characters[i].name, LIST_START_X, y, COLOR_TEXT);

        // El indicador se dibuja solo junto al personaje seleccionado.
        if (i == selectedIndex)
        {
            SDL_Rect dest = { LIST_START_X - INDICATOR_SIZE - 12, y + 4,
                              INDICATOR_SIZE, INDICATOR_SIZE };
            SDL_RenderCopy(res.renderer, res.indicator, nullptr, &dest);
        }
    }
}

void drawLegend(Resources& res)
{
    drawText(res.renderer, res.fontLegend,
        "UP/DOWN: select fighter    SPACE: remove    ESC: exit",
        20, WINDOW_HEIGHT - 40, COLOR_TEXT);
}

// TODO (extras): void drawBackground(Resources& res);
// TODO (extras): void drawCharacterDetails(Resources& res, const Character& c);
//                (imagen a la derecha + atributos del luchador seleccionado)

void renderScreen(Resources& res, const AppState& state)
{
    // 1) Limpia la pantalla con el color de fondo.
    SDL_SetRenderDrawColor(res.renderer, COLOR_BACKGROUND.r, COLOR_BACKGROUND.g,
        COLOR_BACKGROUND.b, COLOR_BACKGROUND.a);
    SDL_RenderClear(res.renderer);

    // 2) Dibuja cada elemento (el orden importa: lo ultimo queda arriba).
    drawTitle(res);
    drawCharacterList(res, state.characters, state.selectedIndex);
    drawLegend(res);

    // 3) Muestra el frame en la ventana.
    SDL_RenderPresent(res.renderer);
}


// =============================================================================
// 6. GAME LOOP: INPUT / UPDATE / RENDER
// =============================================================================

// Mueve el indicador. direction: -1 = arriba, +1 = abajo.
void moveSelection(AppState& state, Resources& res, int direction)
{
    int count = static_cast<int>(state.characters.size());
    if (count == 0)
    {
        return;
    }

    // Navegacion circular: despues del ultimo vuelve al primero y viceversa.
    state.selectedIndex = (state.selectedIndex + direction + count) % count;
    playSound(res.soundMove);
}

// Elimina el personaje seleccionado (de pantalla y del vector).
void removeSelected(AppState& state, Resources& res)
{
    if (!removeCharacter(state.characters, state.selectedIndex))
    {
        return;
    }

    playSound(res.soundRemove);

    // Si se borro el ultimo de la lista, el indicador sube una posicion.
    int count = static_cast<int>(state.characters.size());
    if (state.selectedIndex >= count)
    {
        state.selectedIndex = (count > 0) ? count - 1 : 0;
    }
}

void processInput(AppState& state, Resources& res)
{
    SDL_Event event;
    while (SDL_PollEvent(&event))
    {
        if (event.type == SDL_QUIT)   // boton X de la ventana
        {
            state.running = false;
        }
        else if (event.type == SDL_KEYDOWN && event.key.repeat == 0)
        {
            switch (event.key.keysym.sym)
            {
            case SDLK_UP:     moveSelection(state, res, -1); break;
            case SDLK_DOWN:   moveSelection(state, res, +1); break;
            case SDLK_SPACE:  removeSelected(state, res);    break;
            case SDLK_ESCAPE: state.running = false;         break;
            default: break;
            }
        }
    }
}

void update(AppState& state)
{
    (void)state;
    // TODO (extras): animaciones, logica adicional, etc.
}


// =============================================================================
// 7. INICIO / CIERRE / MAIN
// =============================================================================

bool init(Resources& res, AppState& state)
{
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO) < 0)
    {
        SDL_Log("SDL_Init error: %s", SDL_GetError());
        return false;
    }

    if (!initGraphics(res) || !initAudio(res))
    {
        return false;
    }

    if (!loadCharacters(CHARACTERS_FILE, state.characters))
    {
        SDL_Log("No se pudo abrir %s", CHARACTERS_FILE);
        return false;
    }

    return true;
}

void shutdown(Resources& res)
{
    // Se libera en orden inverso a la creacion, solo lo que se haya creado.
    if (res.soundMove != nullptr)   Mix_FreeChunk(res.soundMove);
    if (res.soundRemove != nullptr) Mix_FreeChunk(res.soundRemove);
    Mix_CloseAudio();

    if (res.indicator != nullptr)   SDL_DestroyTexture(res.indicator);
    if (res.fontTitle != nullptr)   TTF_CloseFont(res.fontTitle);
    if (res.fontItem != nullptr)    TTF_CloseFont(res.fontItem);
    if (res.fontLegend != nullptr)  TTF_CloseFont(res.fontLegend);
    if (res.renderer != nullptr)    SDL_DestroyRenderer(res.renderer);
    if (res.window != nullptr)      SDL_DestroyWindow(res.window);

    IMG_Quit();
    TTF_Quit();
    SDL_Quit();
}

int main(int argc, char* argv[])
{
    (void)argc;
    (void)argv;

    Resources res;
    AppState  state;

    if (!init(res, state))
    {
        shutdown(res);
        return 1;
    }

    // ----- GAME LOOP -----
    while (state.running)
    {
        processInput(state, res);   // 1) leer teclado
        update(state);              // 2) actualizar logica
        renderScreen(res, state);   // 3) dibujar
    }

    shutdown(res);
    return 0;
}