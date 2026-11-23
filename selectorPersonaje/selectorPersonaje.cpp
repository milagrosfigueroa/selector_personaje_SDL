// =============================================================================
// SELECTOR DE PERSONAJES - SDL2   (version con extras)
// Todo el programa en un solo archivo, organizado por secciones:
//   1. Includes y constantes
//   2. Estructuras de datos
//   3. Personajes (archivo + vector + atributos + imagenes)
//   4. Audio
//   5. Graficos (UI)
//   6. Game loop (input / update / render)
//   7. Inicio / cierre / main
//
// EXTRAS agregados (buscar "EXTRA" en el codigo):
//   - Imagen de fondo
//   - Musica de fondo
//   - Panel derecho: imagen + atributos del luchador seleccionado
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

// EXTRA: rutas de los nuevos assets
const char* BACKGROUND_PATH = "assets/images/castillo-background.jpg";
const char* MUSIC_PATH = "assets/sounds/music.wav";
const char* PORTRAITS_DIR = "assets/images/portraits/";   // + nombre + ".png"

const int FONT_SIZE_TITLE = 48;
const int FONT_SIZE_ITEM = 32;
const int FONT_SIZE_LEGEND = 20;

const int LIST_START_X = 120;
const int LIST_START_Y = 140;
const int LIST_ITEM_HEIGHT = 50;
const int INDICATOR_SIZE = 40;

// EXTRA: layout del panel derecho
const int DETAILS_X = 780;
const int PORTRAIT_Y = 100;
const int PORTRAIT_SIZE = 360;
const int DETAILS_NAME_Y = 470;
const int STATS_START_Y = 525;
const int STATS_ROW_HEIGHT = 35;
const int STAT_BAR_OFFSET_X = 110;   // distancia entre la etiqueta y la barra
const int STAT_BAR_WIDTH = 200;
const int STAT_BAR_HEIGHT = 16;
const int STAT_MAX = 100;

// EXTRA: volumen de la musica (0 a MIX_MAX_VOLUME = 128)
const int MUSIC_VOLUME = 40;

// EXTRA: fotitos de los personajes eliminados (debajo de la lista)
const int ELIMINATED_LABEL_Y = 505;
const int ELIMINATED_THUMB_Y = 535;
const int THUMB_SIZE = 64;
const int THUMB_SPACING = 72;                  // ancho de cada casillero
const int THUMBS_PER_ROW = (DETAILS_X - 40 - LIST_START_X) / THUMB_SPACING;

const SDL_Color COLOR_BACKGROUND = { 64, 131, 200, 255 };
const SDL_Color COLOR_TEXT = { 255, 255, 255, 255 };


// =============================================================================
// 2. ESTRUCTURAS DE DATOS
// =============================================================================

// Datos de un luchador.
struct Character
{
    std::string name;

    // EXTRA: atributos e imagen del luchador
    int health = 50;
    int strength = 50;
    int speed = 50;
    SDL_Texture* portrait = nullptr;   // imagen a la derecha de la lista
};

// EXTRA: tabla de atributos por luchador (una fila por personaje)
struct CharacterStats
{
    const char* name;
    int health;
    int strength;
    int speed;
};

const CharacterStats STATS_TABLE[] =
{
    { "Draco Malfoy",        85, 70, 90 },
    { "Cedric Diggory",    80, 85, 85 },
    { "Bellatrix Lestrange",    90, 80, 70 },
    { "Harry Potter",       75, 65, 95 },
    { "Dobby",        85, 80, 60 },
    { "Ron Wesley",       100, 100, 30 },
    { "Albus Dumbledore",      95, 85, 75 },
    { "Hermione Granger", 70, 75, 80 }
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

    // EXTRA
    SDL_Texture* background = nullptr;
    Mix_Music* music = nullptr;
};

// Estado de la aplicacion.
struct AppState
{
    std::vector<Character> characters;   // lista en memoria
    std::vector<Character> eliminated;   // EXTRA: personajes removidos (se muestran abajo)
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

        characters.push_back(character);
    }

    return true;
}

// EXTRA: asigna atributos (desde STATS_TABLE) y carga la imagen del personaje.
// Se llama despues de loadCharacters(), porque necesita el renderer.
void loadCharacterAssets(Character& character, SDL_Renderer* renderer)
{
    // Busca al personaje en la tabla. Si no esta, quedan los valores por defecto.
    for (const CharacterStats& stats : STATS_TABLE)
    {
        if (character.name == stats.name)
        {
            character.health = stats.health;
            character.strength = stats.strength;
            character.speed = stats.speed;
            break;
        }
    }

    // La imagen se busca como assets/images/portraits/<nombre>.png
    std::string path = std::string(PORTRAITS_DIR) + character.name + ".png";
    character.portrait = IMG_LoadTexture(renderer, path.c_str());
    if (character.portrait == nullptr)
    {
        // No es fatal: se dibuja un recuadro gris en su lugar.
        SDL_Log("No se encontro la imagen: %s", path.c_str());
    }
}

// Elimina el personaje en 'index' del vector. Devuelve true si lo elimino.
// EXTRA: antes de borrarlo, guarda una copia en 'eliminated' para mostrar su foto.
// La textura NO se destruye aca: pasa a ser responsabilidad del vector 'eliminated'.
bool removeCharacter(std::vector<Character>& characters,
    std::vector<Character>& eliminated, int index)
{
    if (index < 0 || index >= static_cast<int>(characters.size()))
    {
        return false;
    }

    eliminated.push_back(characters[index]);
    characters.erase(characters.begin() + index);
    return true;
}

// EXTRA: libera las texturas de todos los personajes de un vector.
void freePortraits(std::vector<Character>& characters)
{
    for (Character& character : characters)
    {
        if (character.portrait != nullptr)
        {
            SDL_DestroyTexture(character.portrait);
            character.portrait = nullptr;
        }
    }
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

    // EXTRA: musica de fondo en loop (no es fatal si falta el archivo).
    res.music = Mix_LoadMUS(MUSIC_PATH);
    if (res.music == nullptr)
    {
        SDL_Log("No se pudo cargar la musica: %s", Mix_GetError());
    }
    else
    {
        Mix_VolumeMusic(MUSIC_VOLUME);
        Mix_PlayMusic(res.music, -1);   // -1 = repetir infinitamente
    }

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

    // EXTRA: imagen de fondo (no es fatal: si falta, se usa el color de fondo).
    res.background = IMG_LoadTexture(res.renderer, BACKGROUND_PATH);
    if (res.background == nullptr)
    {
        SDL_Log("No se pudo cargar el fondo: %s", IMG_GetError());
    }

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

// EXTRA: dibuja el fondo (imagen si existe, si no color liso).
void drawBackground(Resources& res)
{
    SDL_SetRenderDrawColor(res.renderer, COLOR_BACKGROUND.r, COLOR_BACKGROUND.g,
        COLOR_BACKGROUND.b, COLOR_BACKGROUND.a);
    SDL_RenderClear(res.renderer);

    if (res.background != nullptr)
    {
        // nullptr, nullptr = toda la imagen estirada a toda la ventana
        SDL_RenderCopy(res.renderer, res.background, nullptr, nullptr);
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

// EXTRA: dibuja una fila de atributo: etiqueta + barra proporcional + numero.
void drawStatBar(Resources& res, const std::string& label, int value, int x, int y)
{
    // Evita que la barra se salga de su fondo.
    if (value < 0) value = 0;
    if (value > STAT_MAX) value = STAT_MAX;

    drawText(res.renderer, res.fontLegend, label, x, y, COLOR_TEXT);

    // Fondo de la barra
    SDL_Rect back = { x + STAT_BAR_OFFSET_X, y + 4, STAT_BAR_WIDTH, STAT_BAR_HEIGHT };
    SDL_SetRenderDrawColor(res.renderer, 30, 60, 100, 255);
    SDL_RenderFillRect(res.renderer, &back);

    // Parte llena (ancho proporcional al valor)
    SDL_Rect fill = back;
    fill.w = STAT_BAR_WIDTH * value / STAT_MAX;
    SDL_SetRenderDrawColor(res.renderer, 240, 200, 40, 255);
    SDL_RenderFillRect(res.renderer, &fill);

    // Valor numerico a la derecha de la barra
    drawText(res.renderer, res.fontLegend, std::to_string(value),
        back.x + STAT_BAR_WIDTH + 12, y, COLOR_TEXT);
}

// EXTRA: panel derecho con imagen, nombre y atributos del luchador seleccionado.
void drawCharacterDetails(Resources& res, const Character& character)
{
    SDL_Rect portraitRect = { DETAILS_X, PORTRAIT_Y, PORTRAIT_SIZE, PORTRAIT_SIZE };

    if (character.portrait != nullptr)
    {
        SDL_RenderCopy(res.renderer, character.portrait, nullptr, &portraitRect);
    }
    else
    {
        // Sin imagen: recuadro gris.
        SDL_SetRenderDrawColor(res.renderer, 90, 90, 90, 255);
        SDL_RenderFillRect(res.renderer, &portraitRect);
    }

    drawText(res.renderer, res.fontItem, character.name, DETAILS_X, DETAILS_NAME_Y, COLOR_TEXT);

    drawStatBar(res, "Health", character.health, DETAILS_X, STATS_START_Y);
    drawStatBar(res, "Strength", character.strength, DETAILS_X, STATS_START_Y + STATS_ROW_HEIGHT);
    drawStatBar(res, "Speed", character.speed, DETAILS_X, STATS_START_Y + 2 * STATS_ROW_HEIGHT);
}

// EXTRA: dibuja las fotitos de los personajes eliminados, debajo de la lista.
void drawEliminated(Resources& res, const std::vector<Character>& eliminated)
{
    if (eliminated.empty())
    {
        return;
    }

    drawText(res.renderer, res.fontLegend, "SELECCIONADOS", LIST_START_X,
        ELIMINATED_LABEL_Y, COLOR_TEXT);

    for (int i = 0; i < static_cast<int>(eliminated.size()); ++i)
    {
        // Posicion en la grilla: pasa a una nueva fila cuando se llena la actual.
        int column = i % THUMBS_PER_ROW;
        int row = i / THUMBS_PER_ROW;

        SDL_Rect dest = { LIST_START_X + column * THUMB_SPACING,
                          ELIMINATED_THUMB_Y + row * THUMB_SPACING,
                          THUMB_SIZE, THUMB_SIZE };

        if (eliminated[i].portrait != nullptr)
        {
            // Oscurece la imagen para que se vea "fuera de combate".
            SDL_SetTextureColorMod(eliminated[i].portrait, 130, 130, 130);
            SDL_RenderCopy(res.renderer, eliminated[i].portrait, nullptr, &dest);
        }
        else
        {
            SDL_SetRenderDrawColor(res.renderer, 90, 90, 90, 255);
            SDL_RenderFillRect(res.renderer, &dest);
        }

        // Borde blanco alrededor de cada fotito.
        SDL_SetRenderDrawColor(res.renderer, 255, 255, 255, 255);
        SDL_RenderDrawRect(res.renderer, &dest);
    }
}

void drawLegend(Resources& res)
{
    drawText(res.renderer, res.fontLegend,
        "UP/DOWN: find fighter    SPACE: select    ESC: exit",
        20, WINDOW_HEIGHT - 40, COLOR_TEXT);
}

void renderScreen(Resources& res, const AppState& state)
{
    // 1) Fondo (tambien limpia la pantalla).
    drawBackground(res);

    // 2) Dibuja cada elemento (el orden importa: lo ultimo queda arriba).
    drawTitle(res);
    drawCharacterList(res, state.characters, state.selectedIndex);
    drawEliminated(res, state.eliminated);   // EXTRA

    // EXTRA: el panel solo se dibuja si queda algun personaje en la lista.
    if (!state.characters.empty())
    {
        drawCharacterDetails(res, state.characters[state.selectedIndex]);
    }

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
    if (!removeCharacter(state.characters, state.eliminated, state.selectedIndex))
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
    // Espacio para logica adicional (animaciones, etc.).
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

    // EXTRA: atributos + imagen de cada personaje.
    for (Character& character : state.characters)
    {
        loadCharacterAssets(character, res.renderer);
    }

    return true;
}

void shutdown(Resources& res, AppState& state)
{
    // EXTRA: libera las imagenes de los que quedaron en la lista y de los eliminados.
    freePortraits(state.characters);
    freePortraits(state.eliminated);

    // Se libera en orden inverso a la creacion, solo lo que se haya creado.
    if (res.music != nullptr) { Mix_HaltMusic(); Mix_FreeMusic(res.music); }
    if (res.soundMove != nullptr)   Mix_FreeChunk(res.soundMove);
    if (res.soundRemove != nullptr) Mix_FreeChunk(res.soundRemove);
    Mix_CloseAudio();

    if (res.background != nullptr)  SDL_DestroyTexture(res.background);
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
        shutdown(res, state);
        return 1;
    }

    // ----- GAME LOOP -----
    while (state.running)
    {
        processInput(state, res);   // 1) leer teclado
        update(state);              // 2) actualizar logica
        renderScreen(res, state);   // 3) dibujar
    }

    shutdown(res, state);
    return 0;
}