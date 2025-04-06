# Требуемая минимальная версия CMake. vcpkg хорошо работает с 3.15 и выше.
cmake_minimum_required(VERSION 3.15)

# Определение проекта: имя 'kpkapp', язык C++
project(kpkapp LANGUAGES CXX)

# Установка стандарта C++17, как требуется в вашем коде
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON) # Требовать именно этот стандарт
set(CMAKE_CXX_EXTENSIONS OFF)      # Отключить расширения компилятора (хорошая практика)

# --- Интеграция с vcpkg ---
# ... (комментарии остаются те же)
# --------------------------

# Поиск пакета sqlite3, установленного через vcpkg, используя имя, которое vcpkg предоставил
find_package(unofficial-sqlite3 CONFIG REQUIRED)

# Добавление исполняемого файла 'kpkapp'.
add_executable(kpkapp main.cpp) # Убедитесь, что имя файла правильное

# Линковка (связывание) вашего исполняемого файла с библиотекой sqlite3.
# Используем импортированную цель, которую vcpkg предоставил
target_link_libraries(kpkapp PRIVATE unofficial::sqlite3::sqlite3)

# Опционально: Включение предупреждений компилятора (рекомендуется)
if(MSVC)
    target_compile_options(kpkapp PRIVATE /W4) # Для MSVC (Visual Studio)
else()
    target_compile_options(kpkapp PRIVATE -Wall -Wextra -pedantic) # Для GCC/Clang
endif()

# Сообщение для пользователя
message(STATUS "Проект 'kpkapp' настроен.")
message(STATUS "Исполняемый файл: ${CMAKE_PROJECT_NAME}")
# Теперь используем переменную от unofficial-sqlite3
message(STATUS "Используется SQLite3: ${unofficial-sqlite3_FOUND}")
