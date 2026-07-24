# Jak Dodać Zewnętrzny Folder do Projektu PlatformIO

## Problem
PlatformIO GUI przestaje działać gdy dodasz drugi folder do workspace w VS Code.

## Rozwiązanie: Directory Junction

### Krok 1: Otwórz Terminal w Projekcie
```powershell
cd c:\JC4827W543_Integrated
```

### Krok 2: Utwórz Junction
```powershell
cmd /c mklink /J nazwa_folderu ścieżka\do\zewnętrznego\folderu
```

**Przykład:**
```powershell
cmd /c mklink /J www D:\www
```

**Wyjaśnienie parametrów:**
- `cmd` - uruchamia Command Prompt (cmd.exe)
- `/c` - wykonaj komendę i zamknij CMD
- `mklink` - komenda do tworzenia linków
- `/J` - typ linku: **Junction** (katalog, bez admin)
- `www` - nazwa linku w projekcie
- `D:\www` - ścieżka do prawdziwego folderu

cmd /c mklink /J www D:\www
│   │  │      │  │   └─ Docelowy folder (prawdziwa lokalizacja)
│   │  │      │  └───── Nazwa linku w projekcie
│   │  │      └──────── Typ: Junction (katalog bez admin)
│   │  └─────────────── Komenda: make link
│   └────────────────── Flaga: wykonaj i zamknij CMD
└────────────────────── Uruchom Command Prompt

### Krok 3: Dodaj do .gitignore
```
# External folder junctions
www/
```

### Krok 4: Gotowe!
Folder `www/` pojawi się w projekcie - możesz edytować pliki normalnie w VS Code.

---

## Usuwanie Junction

```powershell
cmd /c rmdir www
```
**Uwaga:** To usuwa tylko link, nie dane w `D:\www`!

---

## Dlaczego Junction a nie Symlink?
- **Junction (`/J`)** - link dla katalogów, nie wymaga uprawnień administratora
- **Symlink (`/D`)** - wymaga `mklink /D` z admin prawami (Run as Administrator)
- **Hard Link (`/H`)** - tylko dla plików, nie katalogów

### Typy linków mklink:
| Typ | Parametr | Dla | Wymaga Admin? |
|-----|----------|-----|---------------|
| Junction | `/J` | Katalogi | ❌ Nie |
| Symlink (dir) | `/D` | Katalogi | ✅ Tak |
| Symlink (file) | bez flag | Pliki | ✅ Tak |
| Hard Link | `/H` | Pliki | ❌ Nie |

---

## Sprawdzanie Junctions
```powershell
dir | findstr JUNCTION
```

---

## Przykłady Użycia

### Dodaj folder z dokumentacją
```powershell
cmd /c mklink /J docs D:\Documentation\ESP32
```

### Dodaj folder z obrazami
```powershell
cmd /c mklink /J assets E:\Images\Project_Assets
```

### Dodaj bibliotekę
```powershell
cmd /c mklink /J external_lib C:\Libraries\MyLib
```

---

## Co to robi?
1. Tworzy "skrót" folderu w projekcie
2. VS Code widzi go jako normalny folder
3. Edycja plików → zapisuje w oryginalnej lokalizacji
4. PlatformIO GUI dalej działa (single-folder workspace)

---

## Troubleshooting

### "You do not have sufficient privilege"
Użyj `/J` zamiast `/D`:
```powershell
cmd /c mklink /J www D:\www  # ✅ Działa bez admin
```

### Junction nie działa po restarcie
Sprawdź czy dysk docelowy jest podłączony (np. E:, D:)

### Jak zobaczyć gdzie prowadzi junction?
```powershell
cmd /c dir /AL
```
