"""
Polytopia MVP Clone - A polished turn-based 4X strategy game
Requires: pygame (pip install pygame)
"""

import pygame
import random
import math
from enum import Enum, auto
from dataclasses import dataclass, field
from typing import Optional, List, Dict, Tuple, Set
from collections import deque
import colorsys

# =============================================================================
# CONSTANTS & CONFIGURATION
# =============================================================================

WINDOW_WIDTH, WINDOW_HEIGHT = 1280, 800
GRID_SIZE = 14
TILE_SIZE = 48
MAP_OFFSET_X, MAP_OFFSET_Y = 40, 80
MAX_TURNS = 3000
FPS = 60

# Colors
COLORS = {
    'bg': (15, 23, 42),
    'panel': (30, 41, 59),
    'panel_light': (51, 65, 85),
    'text': (248, 250, 252),
    'text_dim': (148, 163, 184),
    'gold': (251, 191, 36),
    'blue_player': (59, 130, 246),
    'red_player': (239, 68, 68),
    'green': (34, 197, 94),
    'purple': (168, 85, 247),
    'highlight_move': (34, 197, 94, 100),
    'highlight_attack': (239, 68, 68, 100),
    'fog': (15, 23, 42),
}

TERRAIN_COLORS = {
    'land': (134, 239, 172),
    'forest': (22, 101, 52),
    'mountain': (120, 113, 108),
    'water': (56, 189, 248),
    'deep_water': (14, 116, 144),
}


# =============================================================================
# ENUMS & DATA CLASSES
# =============================================================================

class Terrain(Enum):
    LAND = auto()
    FOREST = auto()
    MOUNTAIN = auto()
    WATER = auto()

class Resource(Enum):
    NONE = auto()
    GAME = auto()
    FISH = auto()
    FRUIT = auto()
    ORE = auto()

class GamePhase(Enum):
    PLAYING = auto()
    GAME_OVER = auto()
    ANIMATING = auto()

@dataclass
class Tech:
    name: str
    cost: int
    unlocks: List[str]
    requires: Optional[str] = None
    harvest_resource: Optional[Resource] = None
    allows_terrain: Optional[Terrain] = None

@dataclass
class UnitType:
    name: str
    icon: str
    atk: int
    defense: int
    hp: int
    move: int
    attack_range: int
    cost: int
    water_unit: bool = False
    special: Optional[str] = None

@dataclass
class Unit:
    id: int
    type_id: str
    x: int
    y: int
    owner: int
    hp: int
    max_hp: int
    moved: bool = False
    attacked: bool = False
   
    # Animation state
    anim_x: float = 0
    anim_y: float = 0
   
    def __post_init__(self):
        self.anim_x = float(self.x)
        self.anim_y = float(self.y)

@dataclass
class City:
    id: int
    x: int
    y: int
    owner: int
    population: int = 1
    is_capital: bool = False
    level: int = 1

@dataclass
class Tile:
    terrain: Terrain
    resource: Resource = Resource.NONE
    visible: bool = False
    owner: Optional[int] = None
    harvested: bool = False

@dataclass
class Player:
    id: int
    name: str
    color: Tuple[int, int, int]
    stars: int = 5
    techs: List[str] = field(default_factory=list)
    score: int = 0
    is_ai: bool = False

@dataclass
class FloatingText:
    text: str
    x: float
    y: float
    color: Tuple[int, int, int]
    timer: float = 1.0
    vel_y: float = -30

@dataclass
class Particle:
    x: float
    y: float
    vel_x: float
    vel_y: float
    color: Tuple[int, int, int]
    life: float
    size: float


# =============================================================================
# GAME DATA
# =============================================================================

TECHS: Dict[str, Tech] = {
    'hunting': Tech('Hunting', 5, ['archer'], harvest_resource=Resource.GAME),
    'forestry': Tech('Forestry', 5, [], harvest_resource=Resource.FRUIT),
    'fishing': Tech('Fishing', 5, ['boat'], harvest_resource=Resource.FISH),
    'climbing': Tech('Climbing', 5, [], allows_terrain=Terrain.MOUNTAIN),
    'riding': Tech('Riding', 5, ['rider']),
    'shields': Tech('Shields', 6, ['defender']),
    'mining': Tech('Mining', 8, [], requires='climbing', harvest_resource=Resource.ORE),
    'roads': Tech('Roads', 8, [], requires='riding'),
    'smithery': Tech('Smithery', 10, ['swordsman'], requires='mining'),
    'navigation': Tech('Navigation', 12, ['ship'], requires='fishing'),
    'chivalry': Tech('Chivalry', 14, ['knight'], requires='riding'),
    'mathematics': Tech('Mathematics', 15, ['catapult'], requires='smithery'),
}

UNIT_TYPES: Dict[str, UnitType] = {
    'warrior': UnitType('Warrior', '⚔', 2, 2, 10, 1, 1, 2),
    'archer': UnitType('Archer', '🏹', 2, 1, 10, 1, 2, 3),
    'rider': UnitType('Rider', '🐎', 2, 1, 10, 2, 1, 3),
    'defender': UnitType('Defender', '🛡', 1, 3, 15, 1, 1, 3),
    'swordsman': UnitType('Swordsman', '🗡', 3, 3, 15, 1, 1, 5),
    'knight': UnitType('Knight', '🏇', 3, 1, 10, 3, 1, 8, special='persist'),
    'catapult': UnitType('Catapult', '💥', 4, 0, 10, 1, 3, 8),
    'boat': UnitType('Boat', '🚣', 1, 1, 10, 2, 1, 0, water_unit=True),
    'ship': UnitType('Ship', '⛵', 2, 2, 15, 3, 2, 5, water_unit=True),
    'giant': UnitType('Giant', '🗿', 5, 4, 40, 1, 1, 0),
}

RESOURCE_ICONS = {
    Resource.GAME: '🦌',
    Resource.FISH: '🐟',
    Resource.FRUIT: '🍎',
    Resource.ORE: '�ite',
}


# =============================================================================
# GAME ENGINE
# =============================================================================

class Game:
    def __init__(self):
        pygame.init()
        pygame.display.set_caption("Polytopia MVP")
        self.screen = pygame.display.set_mode((WINDOW_WIDTH, WINDOW_HEIGHT))
        self.clock = pygame.time.Clock()
       
        # Fonts
        self.font_large = pygame.font.Font(None, 48)
        self.font_medium = pygame.font.Font(None, 32)
        self.font_small = pygame.font.Font(None, 24)
        self.font_tiny = pygame.font.Font(None, 18)
       
        # Try to load emoji font, fallback to symbols
        try:
            self.font_emoji = pygame.font.Font(None, 28)
        except:
            self.font_emoji = self.font_medium
       
        self.running = True
        self.floating_texts: List[FloatingText] = []
        self.particles: List[Particle] = []
        self.camera_shake = 0
        self.message = "Welcome to Polytopia! Select units to move, cities to train."
        self.message_timer = 5.0
       
        self.new_game()
   
    def new_game(self):
        """Initialize a new game state."""
        self.tiles: List[List[Tile]] = self.generate_map()
        self.players = [
            Player(0, "Blue Kingdom", COLORS['blue_player'], stars=5),
            Player(1, "Red Empire", COLORS['red_player'], stars=5, is_ai=True),
        ]
       
        # Starting positions
        start_positions = [(2, 2), (GRID_SIZE - 3, GRID_SIZE - 3)]
       
        self.cities: List[City] = []
        self.units: List[Unit] = []
        self.next_unit_id = 0
        self.next_city_id = 0
       
        for i, (px, py) in enumerate(start_positions):
            # Create capital city
            city = City(self.next_city_id, px, py, i, population=1, is_capital=True)
            self.cities.append(city)
            self.next_city_id += 1
            self.tiles[py][px].owner = i
           
            # Create starting warrior
            unit = self.create_unit('warrior', px + 1, py, i)
            self.units.append(unit)
           
            # Reveal starting area
            self.reveal_area(px, py, 3, i)
       
        self.turn = 1
        self.current_player = 0
        self.phase = GamePhase.PLAYING
        self.winner = None
       
        # UI State
        self.selected_unit: Optional[Unit] = None
        self.selected_city: Optional[City] = None
        self.show_tech_panel = False
        self.show_train_panel = False
        self.move_highlights: Set[Tuple[int, int]] = set()
        self.attack_highlights: Set[Tuple[int, int]] = set()
       
        # Animation
        self.animating_unit: Optional[Unit] = None
        self.animation_path: List[Tuple[int, int]] = []
        self.animation_progress = 0
   
    def generate_map(self) -> List[List[Tile]]:
        """Generate procedural terrain map."""
        tiles = [[Tile(Terrain.LAND) for _ in range(GRID_SIZE)] for _ in range(GRID_SIZE)]
       
        # Generate terrain using noise-like patterns
        # Create water bodies
        num_water = random.randint(2, 4)
        for _ in range(num_water):
            cx, cy = random.randint(2, GRID_SIZE-3), random.randint(2, GRID_SIZE-3)
            size = random.randint(2, 4)
            for dy in range(-size, size+1):
                for dx in range(-size, size+1):
                    nx, ny = cx + dx, cy + dy
                    if 0 <= nx < GRID_SIZE and 0 <= ny < GRID_SIZE:
                        dist = math.sqrt(dx*dx + dy*dy)
                        if dist < size and random.random() < 0.7:
                            tiles[ny][nx].terrain = Terrain.WATER
       
        # Add forests
        for y in range(GRID_SIZE):
            for x in range(GRID_SIZE):
                if tiles[y][x].terrain == Terrain.LAND and random.random() < 0.25:
                    tiles[y][x].terrain = Terrain.FOREST
       
        # Add mountains
        num_mountains = random.randint(2, 4)
        for _ in range(num_mountains):
            cx, cy = random.randint(1, GRID_SIZE-2), random.randint(1, GRID_SIZE-2)
            for dy in range(-1, 2):
                for dx in range(-1, 2):
                    nx, ny = cx + dx, cy + dy
                    if 0 <= nx < GRID_SIZE and 0 <= ny < GRID_SIZE:
                        if tiles[ny][nx].terrain == Terrain.LAND and random.random() < 0.5:
                            tiles[ny][nx].terrain = Terrain.MOUNTAIN
       
        # Add resources
        for y in range(GRID_SIZE):
            for x in range(GRID_SIZE):
                tile = tiles[y][x]
                if tile.terrain == Terrain.FOREST and random.random() < 0.5:
                    tile.resource = random.choice([Resource.GAME, Resource.FRUIT])
                elif tile.terrain == Terrain.WATER and random.random() < 0.4:
                    tile.resource = Resource.FISH
                elif tile.terrain == Terrain.MOUNTAIN and random.random() < 0.5:
                    tile.resource = Resource.ORE
       
        # Ensure starting positions are clear
        for px, py in [(2, 2), (GRID_SIZE-3, GRID_SIZE-3)]:
            for dy in range(-1, 2):
                for dx in range(-1, 2):
                    nx, ny = px + dx, py + dy
                    if 0 <= nx < GRID_SIZE and 0 <= ny < GRID_SIZE:
                        if tiles[ny][nx].terrain == Terrain.WATER:
                            tiles[ny][nx].terrain = Terrain.LAND
                        tiles[ny][nx].resource = Resource.NONE
       
        return tiles
   
    def create_unit(self, type_id: str, x: int, y: int, owner: int) -> Unit:
        """Create a new unit."""
        unit_type = UNIT_TYPES[type_id]
        unit = Unit(
            self.next_unit_id, type_id, x, y, owner,
            hp=unit_type.hp, max_hp=unit_type.hp
        )
        self.next_unit_id += 1
        return unit
   
    def reveal_area(self, cx: int, cy: int, radius: int, player: int = None):
        """Reveal tiles around a position."""
        for dy in range(-radius, radius + 1):
            for dx in range(-radius, radius + 1):
                nx, ny = cx + dx, cy + dy
                if 0 <= nx < GRID_SIZE and 0 <= ny < GRID_SIZE:
                    if abs(dx) + abs(dy) <= radius + 1:
                        self.tiles[ny][nx].visible = True
   
    def get_unit_at(self, x: int, y: int) -> Optional[Unit]:
        """Get unit at position."""
        for u in self.units:
            if u.x == x and u.y == y:
                return u
        return None
   
    def get_city_at(self, x: int, y: int) -> Optional[City]:
        """Get city at position."""
        for c in self.cities:
            if c.x == x and c.y == y:
                return c
        return None
   
    def get_current_player(self) -> Player:
        return self.players[self.current_player]
   
    def can_traverse(self, x: int, y: int, unit: Unit) -> bool:
        """Check if unit can move to tile."""
        if not (0 <= x < GRID_SIZE and 0 <= y < GRID_SIZE):
            return False
       
        tile = self.tiles[y][x]
        unit_type = UNIT_TYPES[unit.type_id]
        player = self.players[unit.owner]
       
        if unit_type.water_unit:
            return tile.terrain == Terrain.WATER
       
        if tile.terrain == Terrain.WATER:
            return 'fishing' in player.techs
        if tile.terrain == Terrain.MOUNTAIN:
            return 'climbing' in player.techs
       
        return True
   
    def get_movement_range(self, unit: Unit) -> Set[Tuple[int, int]]:
        """Get all tiles unit can move to."""
        unit_type = UNIT_TYPES[unit.type_id]
        valid = set()
       
        # BFS for movement
        queue = deque([(unit.x, unit.y, unit_type.move)])
        visited = {(unit.x, unit.y)}
       
        while queue:
            x, y, moves_left = queue.popleft()
           
            if moves_left <= 0:
                continue
           
            for dx, dy in [(-1, 0), (1, 0), (0, -1), (0, 1)]:
                nx, ny = x + dx, y + dy
               
                if (nx, ny) in visited:
                    continue
                if not self.can_traverse(nx, ny, unit):
                    continue
               
                visited.add((nx, ny))
                occupant = self.get_unit_at(nx, ny)
               
                if occupant is None:
                    valid.add((nx, ny))
                    queue.append((nx, ny, moves_left - 1))
                elif occupant.owner != unit.owner:
                    valid.add((nx, ny))  # Can attack
       
        return valid
   
    def get_attack_range(self, unit: Unit) -> Set[Tuple[int, int]]:
        """Get all tiles unit can attack."""
        unit_type = UNIT_TYPES[unit.type_id]
        valid = set()
       
        for dy in range(-unit_type.attack_range, unit_type.attack_range + 1):
            for dx in range(-unit_type.attack_range, unit_type.attack_range + 1):
                dist = abs(dx) + abs(dy)
                if 0 < dist <= unit_type.attack_range:
                    nx, ny = unit.x + dx, unit.y + dy
                    if 0 <= nx < GRID_SIZE and 0 <= ny < GRID_SIZE:
                        target = self.get_unit_at(nx, ny)
                        if target and target.owner != unit.owner:
                            valid.add((nx, ny))
       
        return valid
   
    def move_unit(self, unit: Unit, x: int, y: int):
        """Move unit to new position."""
        unit.x = x
        unit.y = y
        unit.moved = True
        self.reveal_area(x, y, 2)
       
        # Check for city capture
        city = self.get_city_at(x, y)
        if city and city.owner != unit.owner:
            old_owner = city.owner
            city.owner = unit.owner
            self.tiles[y][x].owner = unit.owner
            self.spawn_particles(x, y, self.players[unit.owner].color, 20)
            self.add_floating_text("City Captured!", x, y, COLORS['gold'])
            self.set_message(f"{self.players[unit.owner].name} captured a city!")
   
    def attack_unit(self, attacker: Unit, target: Unit):
        """Execute attack between units."""
        atk_type = UNIT_TYPES[attacker.type_id]
        def_type = UNIT_TYPES[target.type_id]
       
        # Damage calculation
        atk_ratio = attacker.hp / attacker.max_hp
        damage = max(1, round(atk_type.atk * atk_ratio * 4.5 - def_type.defense))
       
        target.hp -= damage
        attacker.attacked = True
       
        # Visual feedback
        self.camera_shake = 8
        self.spawn_particles(target.x, target.y, (255, 100, 100), 15)
        self.add_floating_text(f"-{damage}", target.x, target.y, (255, 80, 80))
       
        # Check for kill
        if target.hp <= 0:
            self.units.remove(target)
            self.spawn_particles(target.x, target.y, (255, 200, 100), 25)
            self.add_floating_text("Defeated!", target.x, target.y, COLORS['gold'])
           
            # Knight persist - move to target tile
            if atk_type.special == 'persist':
                attacker.moved = False
                self.move_unit(attacker, target.x, target.y)
        else:
            # Counter attack if in range and melee
            if abs(attacker.x - target.x) + abs(attacker.y - target.y) <= 1:
                counter_damage = max(1, round(def_type.atk * (target.hp / target.max_hp) * 2 - atk_type.defense * 0.5))
                attacker.hp -= counter_damage
                self.add_floating_text(f"-{counter_damage}", attacker.x, attacker.y, (255, 150, 150))
               
                if attacker.hp <= 0:
                    self.units.remove(attacker)
                    self.spawn_particles(attacker.x, attacker.y, (255, 200, 100), 25)
       
        attacker.moved = True
   
    def train_unit(self, city: City, type_id: str):
        """Train a new unit at city."""
        unit_type = UNIT_TYPES[type_id]
        player = self.get_current_player()
       
        if player.stars < unit_type.cost:
            self.set_message("Not enough stars!")
            return False
       
        if self.get_unit_at(city.x, city.y):
            self.set_message("City tile is occupied!")
            return False
       
        player.stars -= unit_type.cost
        unit = self.create_unit(type_id, city.x, city.y, self.current_player)
        unit.moved = True
        unit.attacked = True
        self.units.append(unit)
       
        self.spawn_particles(city.x, city.y, player.color, 10)
        self.add_floating_text(f"+{unit_type.name}", city.x, city.y, COLORS['green'])
        self.set_message(f"Trained {unit_type.name}!")
        return True
   
    def research_tech(self, tech_id: str):
        """Research a technology."""
        tech = TECHS[tech_id]
        player = self.get_current_player()
       
        if tech_id in player.techs:
            return False
        if player.stars < tech.cost:
            self.set_message("Not enough stars!")
            return False
        if tech.requires and tech.requires not in player.techs:
            self.set_message(f"Requires {TECHS[tech.requires].name} first!")
            return False
       
        player.stars -= tech.cost
        player.techs.append(tech_id)
       
        self.add_floating_text(f"Researched {tech.name}!", GRID_SIZE // 2, 1, COLORS['purple'])
        self.set_message(f"Researched {tech.name}!")
        return True
   
    def harvest_resource(self, x: int, y: int):
        """Harvest resource at tile."""
        tile = self.tiles[y][x]
        player = self.get_current_player()
       
        if tile.harvested or tile.resource == Resource.NONE:
            return False
       
        # Check if player has tech
        can_harvest = False
        if tile.resource == Resource.GAME and 'hunting' in player.techs:
            can_harvest = True
        elif tile.resource == Resource.FISH and 'fishing' in player.techs:
            can_harvest = True
        elif tile.resource == Resource.FRUIT and 'forestry' in player.techs:
            can_harvest = True
        elif tile.resource == Resource.ORE and 'mining' in player.techs:
            can_harvest = True
       
        if not can_harvest:
            self.set_message("Need technology to harvest this resource!")
            return False
       
        tile.harvested = True
       
        # Find nearest owned city and grow population
        owned_cities = [c for c in self.cities if c.owner == self.current_player]
        if owned_cities:
            nearest = min(owned_cities, key=lambda c: abs(c.x - x) + abs(c.y - y))
            nearest.population += 1
            self.add_floating_text("+1 Pop", nearest.x, nearest.y, COLORS['green'])
            self.set_message("Harvested! City population increased.")
       
        self.spawn_particles(x, y, COLORS['gold'], 8)
        return True
   
    def calculate_income(self, player_id: int) -> int:
        """Calculate star income for player."""
        cities = [c for c in self.cities if c.owner == player_id]
        return sum(c.population for c in cities) + 1
   
    def calculate_score(self, player_id: int) -> int:
        """Calculate score for player."""
        cities = [c for c in self.cities if c.owner == player_id]
        units = [u for u in self.units if u.owner == player_id]
       
        city_score = sum(c.population * 100 for c in cities)
        unit_score = len(units) * 10
        tech_score = len(self.players[player_id].techs) * 20
       
        return city_score + unit_score + tech_score
   
    def end_turn(self):
        """End current player's turn."""
        player = self.get_current_player()
       
        # Collect income
        income = self.calculate_income(self.current_player)
        player.stars += income
        self.add_floating_text(f"+{income} ⭐", 2, 0, COLORS['gold'])
       
        # Update scores
        for p in self.players:
            p.score = self.calculate_score(p.id)
       
        # Reset unit states
        for u in self.units:
            if u.owner == (self.current_player + 1) % 2:
                u.moved = False
                u.attacked = False
       
        # Switch player
        self.current_player = (self.current_player + 1) % 2
       
        # Increment turn if back to player 0
        if self.current_player == 0:
            self.turn += 1
       
        # Check victory conditions
        self.check_victory()
       
        # Clear selection
        self.selected_unit = None
        self.selected_city = None
        self.move_highlights.clear()
        self.attack_highlights.clear()
        self.show_tech_panel = False
        self.show_train_panel = False
       
        # AI turn
        if self.phase == GamePhase.PLAYING and self.get_current_player().is_ai:
            self.ai_turn()
   
    def check_victory(self):
        """Check for victory conditions."""
        # Check capital capture
        capitals = {0: 0, 1: 0}
        for city in self.cities:
            if city.is_capital:
                capitals[city.owner] += 1
       
        for player_id, count in capitals.items():
            if count == 2:
                self.phase = GamePhase.GAME_OVER
                self.winner = player_id
                return
       
        # Check turn limit
        if self.turn > MAX_TURNS:
            self.phase = GamePhase.GAME_OVER
            self.winner = 0 if self.players[0].score >= self.players[1].score else 1
   
    def ai_turn(self):
        """Simple AI logic."""
        player = self.get_current_player()
       
        # Move and attack with units
        for unit in [u for u in self.units if u.owner == self.current_player]:
            if unit.attacked:
                continue
           
            # Try to attack
            attacks = self.get_attack_range(unit)
            if attacks:
                target_pos = random.choice(list(attacks))
                target = self.get_unit_at(*target_pos)
                if target:
                    self.attack_unit(unit, target)
                    continue
           
            # Try to move toward enemy
            if not unit.moved:
                moves = self.get_movement_range(unit)
                if moves:
                    # Find enemy cities/units
                    enemies = [(c.x, c.y) for c in self.cities if c.owner != self.current_player]
                    enemies += [(u.x, u.y) for u in self.units if u.owner != self.current_player]
                   
                    if enemies:
                        # Move toward nearest enemy
                        best_move = min(moves, key=lambda m: min(
                            abs(m[0] - e[0]) + abs(m[1] - e[1]) for e in enemies
                        ))
                       
                        # Check if moving to attack
                        target = self.get_unit_at(*best_move)
                        if target and target.owner != unit.owner:
                            self.attack_unit(unit, target)
                        else:
                            self.move_unit(unit, *best_move)
       
        # Research tech
        available_techs = [
            tid for tid, t in TECHS.items()
            if tid not in player.techs
            and player.stars >= t.cost
            and (t.requires is None or t.requires in player.techs)
        ]
        if available_techs and random.random() < 0.5:
            self.research_tech(random.choice(available_techs))
       
        # Train units
        for city in [c for c in self.cities if c.owner == self.current_player]:
            if not self.get_unit_at(city.x, city.y):
                available_units = self.get_available_units()
                affordable = [u for u in available_units if UNIT_TYPES[u].cost <= player.stars]
                if affordable:
                    self.train_unit(city, random.choice(affordable))
       
        # End AI turn
        self.end_turn()
   
    def get_available_units(self) -> List[str]:
        """Get units available to current player."""
        player = self.get_current_player()
        available = ['warrior']
       
        for tech_id in player.techs:
            tech = TECHS[tech_id]
            available.extend(tech.unlocks)
       
        return list(set(available))
   
    # =========================================================================
    # VISUAL EFFECTS
    # =========================================================================
   
    def add_floating_text(self, text: str, tile_x: int, tile_y: int, color: Tuple[int, int, int]):
        """Add floating text effect."""
        x = MAP_OFFSET_X + tile_x * TILE_SIZE + TILE_SIZE // 2
        y = MAP_OFFSET_Y + tile_y * TILE_SIZE
        self.floating_texts.append(FloatingText(text, x, y, color))
   
    def spawn_particles(self, tile_x: int, tile_y: int, color: Tuple[int, int, int], count: int):
        """Spawn particle effects."""
        x = MAP_OFFSET_X + tile_x * TILE_SIZE + TILE_SIZE // 2
        y = MAP_OFFSET_Y + tile_y * TILE_SIZE + TILE_SIZE // 2
       
        for _ in range(count):
            angle = random.uniform(0, math.pi * 2)
            speed = random.uniform(50, 150)
            self.particles.append(Particle(
                x, y,
                math.cos(angle) * speed,
                math.sin(angle) * speed,
                color,
                random.uniform(0.3, 0.8),
                random.uniform(2, 5)
            ))
   
    def set_message(self, msg: str):
        """Set status message."""
        self.message = msg
        self.message_timer = 4.0
   
    # =========================================================================
    # INPUT HANDLING
    # =========================================================================
   
    def handle_click(self, pos: Tuple[int, int], button: int):
        """Handle mouse click."""
        if self.phase == GamePhase.GAME_OVER:
            # Click anywhere to restart
            self.new_game()
            return
       
        if self.get_current_player().is_ai:
            return
       
        mx, my = pos
       
        # Check UI panels first
        if self.handle_ui_click(mx, my, button):
            return
       
        # Convert to tile coordinates
        tx = (mx - MAP_OFFSET_X) // TILE_SIZE
        ty = (my - MAP_OFFSET_Y) // TILE_SIZE
       
        if not (0 <= tx < GRID_SIZE and 0 <= ty < GRID_SIZE):
            return
       
        # Right click to harvest
        if button == 3:
            self.harvest_resource(tx, ty)
            return
       
        # Handle unit/city selection and movement
        if self.selected_unit:
            # Try to move or attack
            if (tx, ty) in self.move_highlights:
                target = self.get_unit_at(tx, ty)
                if target and target.owner != self.selected_unit.owner:
                    self.attack_unit(self.selected_unit, target)
                else:
                    self.move_unit(self.selected_unit, tx, ty)
                self.clear_selection()
                return
           
            if (tx, ty) in self.attack_highlights and not self.selected_unit.attacked:
                target = self.get_unit_at(tx, ty)
                if target:
                    self.attack_unit(self.selected_unit, target)
                self.clear_selection()
                return
       
        # Select unit
        unit = self.get_unit_at(tx, ty)
        if unit and unit.owner == self.current_player:
            self.select_unit(unit)
            return
       
        # Select city
        city = self.get_city_at(tx, ty)
        if city and city.owner == self.current_player:
            self.select_city(city)
            return
       
        self.clear_selection()
   
    def handle_ui_click(self, mx: int, my: int, button: int) -> bool:
        """Handle clicks on UI elements. Returns True if handled."""
        # End Turn button
        if WINDOW_WIDTH - 130 <= mx <= WINDOW_WIDTH - 10 and 20 <= my <= 55:
            self.end_turn()
            return True
       
        # Tech button
        if WINDOW_WIDTH - 260 <= mx <= WINDOW_WIDTH - 140 and 20 <= my <= 55:
            self.show_tech_panel = not self.show_tech_panel
            self.show_train_panel = False
            return True
       
        # Tech panel
        if self.show_tech_panel:
            panel_x = WINDOW_WIDTH - 320
            panel_y = 70
            if panel_x <= mx <= panel_x + 300:
                tech_index = (my - panel_y - 10) // 32
                tech_list = list(TECHS.keys())
                if 0 <= tech_index < len(tech_list):
                    self.research_tech(tech_list[tech_index])
                    return True
       
        # Train panel
        if self.show_train_panel and self.selected_city:
            panel_x = MAP_OFFSET_X + GRID_SIZE * TILE_SIZE + 20
            panel_y = 150
            available = [u for u in self.get_available_units() if not UNIT_TYPES[u].water_unit]
            unit_index = (my - panel_y - 10) // 40
            if 0 <= unit_index < len(available):
                self.train_unit(self.selected_city, available[unit_index])
                return True
       
        return False
   
    def select_unit(self, unit: Unit):
        """Select a unit."""
        self.selected_unit = unit
        self.selected_city = None
        self.show_train_panel = False
       
        self.move_highlights = self.get_movement_range(unit) if not unit.moved else set()
        self.attack_highlights = self.get_attack_range(unit) if not unit.attacked else set()
       
        unit_type = UNIT_TYPES[unit.type_id]
        self.set_message(f"Selected {unit_type.name} (HP: {unit.hp}/{unit.max_hp})")
   
    def select_city(self, city: City):
        """Select a city."""
        self.selected_city = city
        self.selected_unit = None
        self.move_highlights.clear()
        self.attack_highlights.clear()
        self.show_train_panel = True
        self.show_tech_panel = False
       
        self.set_message(f"City (Pop: {city.population}) - Select unit to train")
   
    def clear_selection(self):
        """Clear all selections."""
        self.selected_unit = None
        self.selected_city = None
        self.move_highlights.clear()
        self.attack_highlights.clear()
        self.show_train_panel = False
   
    # =========================================================================
    # RENDERING
    # =========================================================================
   
    def render(self):
        """Main render function."""
        # Camera shake offset
        # shake_x = random.randint(-self.camera_shake, self.camera_shake) if self.camera_shake > 0 else 0
        
        # shake_y = random.randint(-self.camera_shake, self.camera_shake) if self.camera_shake > 0 else 0
       
       
        self.screen.fill(COLORS['bg'])
       
        # Render map
        self.render_map(0, 0)
       
        # Render UI
        self.render_ui()
       
        # Render particles
        self.render_particles()
       
        # Render floating text
        self.render_floating_text()
       
        # Game over overlay
        if self.phase == GamePhase.GAME_OVER:
            self.render_game_over()
       
        pygame.display.flip()
   
    def render_map(self, offset_x: int = 0, offset_y: int = 0):
        """Render the game map."""
        for y in range(GRID_SIZE):
            for x in range(GRID_SIZE):
                tile = self.tiles[y][x]
                px = MAP_OFFSET_X + x * TILE_SIZE + offset_x
                py = MAP_OFFSET_Y + y * TILE_SIZE + offset_y
               
                # Base terrain
                if tile.visible:
                    if tile.terrain == Terrain.LAND:
                        color = TERRAIN_COLORS['land']
                    elif tile.terrain == Terrain.FOREST:
                        color = TERRAIN_COLORS['forest']
                    elif tile.terrain == Terrain.MOUNTAIN:
                        color = TERRAIN_COLORS['mountain']
                    else:
                        color = TERRAIN_COLORS['water']
                else:
                    color = COLORS['fog']
               
                pygame.draw.rect(self.screen, color, (px, py, TILE_SIZE - 1, TILE_SIZE - 1))
               
                # Territory indicator
                if tile.visible and tile.owner is not None:
                    owner_color = self.players[tile.owner].color
                    pygame.draw.rect(self.screen, owner_color, (px, py + TILE_SIZE - 4, TILE_SIZE - 1, 3))
               
                # Highlights
                if (x, y) in self.move_highlights:
                    s = pygame.Surface((TILE_SIZE - 1, TILE_SIZE - 1), pygame.SRCALPHA)
                    s.fill((34, 197, 94, 80))
                    self.screen.blit(s, (px, py))
               
                if (x, y) in self.attack_highlights:
                    s = pygame.Surface((TILE_SIZE - 1, TILE_SIZE - 1), pygame.SRCALPHA)
                    s.fill((239, 68, 68, 80))
                    self.screen.blit(s, (px, py))
               
                # Resource
                if tile.visible and tile.resource != Resource.NONE and not tile.harvested:
                    icon = RESOURCE_ICONS.get(tile.resource, '?')
                    txt = self.font_tiny.render(icon, True, COLORS['text'])
                    self.screen.blit(txt, (px + TILE_SIZE - 16, py + 2))
       
        # Render cities
        for city in self.cities:
            if not self.tiles[city.y][city.x].visible:
                continue
           
            px = MAP_OFFSET_X + city.x * TILE_SIZE + offset_x
            py = MAP_OFFSET_Y + city.y * TILE_SIZE + offset_y
           
            color = self.players[city.owner].color
           
            # City base
            pygame.draw.rect(self.screen, color, (px + 8, py + 8, TILE_SIZE - 17, TILE_SIZE - 17))
           
            # Capital indicator
            if city.is_capital:
                pygame.draw.rect(self.screen, COLORS['gold'], (px + 8, py + 8, TILE_SIZE - 17, TILE_SIZE - 17), 2)
           
            # Population
            txt = self.font_tiny.render(str(city.population), True, COLORS['text'])
            self.screen.blit(txt, (px + TILE_SIZE // 2 - txt.get_width() // 2, py + TILE_SIZE // 2 - txt.get_height() // 2))
           
            # Selection
            if self.selected_city and self.selected_city.id == city.id:
                pygame.draw.rect(self.screen, COLORS['gold'], (px + 4, py + 4, TILE_SIZE - 9, TILE_SIZE - 9), 2)
       
        # Render units
        for unit in self.units:
            if not self.tiles[unit.y][unit.x].visible:
                continue
           
            px = MAP_OFFSET_X + unit.anim_x * TILE_SIZE + offset_x
            py = MAP_OFFSET_Y + unit.anim_y * TILE_SIZE + offset_y
           
            unit_type = UNIT_TYPES[unit.type_id]
            color = self.players[unit.owner].color
           
            # Unit circle
            cx, cy = px + TILE_SIZE // 2, py + TILE_SIZE // 2
            pygame.draw.circle(self.screen, color, (int(cx), int(cy)), 16)
            pygame.draw.circle(self.screen, COLORS['text'], (int(cx), int(cy)), 16, 2)
           
            # Unit letter
            txt = self.font_small.render(unit_type.name[0], True, COLORS['text'])
            self.screen.blit(txt, (cx - txt.get_width() // 2, cy - txt.get_height() // 2))
           
            # HP bar
            hp_ratio = unit.hp / unit.max_hp
            hp_color = COLORS['green'] if hp_ratio > 0.5 else COLORS['gold'] if hp_ratio > 0.25 else COLORS['red_player']
            pygame.draw.rect(self.screen, (40, 40, 40), (px + 4, py + TILE_SIZE - 8, TILE_SIZE - 9, 4))
            pygame.draw.rect(self.screen, hp_color, (px + 4, py + TILE_SIZE - 8, int((TILE_SIZE - 9) * hp_ratio), 4))
           
            # Selection
            if self.selected_unit and self.selected_unit.id == unit.id:
                pygame.draw.circle(self.screen, COLORS['gold'], (int(cx), int(cy)), 20, 2)
           
            # Moved indicator
            if unit.moved and unit.owner == self.current_player:
                pygame.draw.circle(self.screen, (100, 100, 100), (int(px + TILE_SIZE - 8), int(py + 8)), 4)
   
    def render_ui(self):
        """Render UI elements."""
        player = self.get_current_player()
       
        # Top bar
        pygame.draw.rect(self.screen, COLORS['panel'], (0, 0, WINDOW_WIDTH, 65))
       
        # Player name and stars
        txt = self.font_medium.render(f"{player.name}", True, player.color)
        self.screen.blit(txt, (20, 15))
       
        txt = self.font_medium.render(f"⭐ {player.stars}", True, COLORS['gold'])
        self.screen.blit(txt, (200, 15))
       
        txt = self.font_small.render(f"+{self.calculate_income(self.current_player)}/turn", True, COLORS['text_dim'])
        self.screen.blit(txt, (270, 20))
       
        # Turn counter
        txt = self.font_medium.render(f"Turn {self.turn}/{MAX_TURNS}", True, COLORS['text'])
        self.screen.blit(txt, (400, 15))
       
        # Scores
        txt = self.font_small.render(f"Blue: {self.players[0].score}", True, COLORS['blue_player'])
        self.screen.blit(txt, (550, 12))
        txt = self.font_small.render(f"Red: {self.players[1].score}", True, COLORS['red_player'])
        self.screen.blit(txt, (550, 32))
       
        # Tech button
        btn_color = COLORS['purple'] if self.show_tech_panel else COLORS['panel_light']
        pygame.draw.rect(self.screen, btn_color, (WINDOW_WIDTH - 260, 20, 110, 35), border_radius=5)
        txt = self.font_small.render("Tech [T]", True, COLORS['text'])
        self.screen.blit(txt, (WINDOW_WIDTH - 235, 28))
       
        # End turn button
        pygame.draw.rect(self.screen, COLORS['green'], (WINDOW_WIDTH - 130, 20, 110, 35), border_radius=5)
        txt = self.font_small.render("End Turn", True, COLORS['text'])
        self.screen.blit(txt, (WINDOW_WIDTH - 110, 28))
       
        # Message bar
        if self.message_timer > 0:
            alpha = min(255, int(self.message_timer * 255))
            txt = self.font_small.render(self.message, True, COLORS['gold'])
            self.screen.blit(txt, (MAP_OFFSET_X, WINDOW_HEIGHT - 30))
       
        # Tech panel
        if self.show_tech_panel:
            self.render_tech_panel()
       
        # Train panel
        if self.show_train_panel:
            self.render_train_panel()
       
        # Help text
        help_y = MAP_OFFSET_Y + GRID_SIZE * TILE_SIZE + 20
        help_texts = [
            "Left-click: Select/Move/Attack",
            "Right-click: Harvest resource",
            "T: Toggle tech panel",
            "Space: End turn"
        ]
        for i, txt in enumerate(help_texts):
            rendered = self.font_tiny.render(txt, True, COLORS['text_dim'])
            self.screen.blit(rendered, (MAP_OFFSET_X + i * 200, help_y))
   
    def render_tech_panel(self):
        """Render technology panel."""
        panel_x = WINDOW_WIDTH - 320
        panel_y = 70
        panel_w = 300
        panel_h = len(TECHS) * 32 + 40
       
        # Background
        pygame.draw.rect(self.screen, COLORS['panel'], (panel_x, panel_y, panel_w, panel_h), border_radius=8)
        pygame.draw.rect(self.screen, COLORS['panel_light'], (panel_x, panel_y, panel_w, panel_h), 2, border_radius=8)
       
        # Title
        txt = self.font_medium.render("Technology", True, COLORS['text'])
        self.screen.blit(txt, (panel_x + 10, panel_y + 5))
       
        player = self.get_current_player()
        y = panel_y + 40
       
        for tech_id, tech in TECHS.items():
            owned = tech_id in player.techs
            can_research = (
                not owned and
                player.stars >= tech.cost and
                (tech.requires is None or tech.requires in player.techs)
            )
           
            # Background
            if owned:
                color = (34, 87, 34)
            elif can_research:
                color = COLORS['panel_light']
            else:
                color = (40, 40, 50)
           
            pygame.draw.rect(self.screen, color, (panel_x + 5, y, panel_w - 10, 28), border_radius=4)
           
            # Text
            txt_color = COLORS['text'] if (owned or can_research) else COLORS['text_dim']
            txt = self.font_tiny.render(f"{tech.name} ({tech.cost}⭐)", True, txt_color)
            self.screen.blit(txt, (panel_x + 12, y + 6))
           
            # Unlocks
            if tech.unlocks:
                unlock_txt = self.font_tiny.render(f"→ {', '.join(tech.unlocks)}", True, COLORS['text_dim'])
                self.screen.blit(unlock_txt, (panel_x + 150, y + 6))
           
            y += 32
   
    def render_train_panel(self):
        """Render unit training panel."""
        panel_x = MAP_OFFSET_X + GRID_SIZE * TILE_SIZE + 20
        panel_y = 150
        panel_w = 200
       
        available = [u for u in self.get_available_units() if not UNIT_TYPES[u].water_unit]
        panel_h = len(available) * 40 + 50
       
        # Background
        pygame.draw.rect(self.screen, COLORS['panel'], (panel_x, panel_y, panel_w, panel_h), border_radius=8)
        pygame.draw.rect(self.screen, COLORS['panel_light'], (panel_x, panel_y, panel_w, panel_h), 2, border_radius=8)
       
        # Title
        txt = self.font_medium.render("Train Unit", True, COLORS['text'])
        self.screen.blit(txt, (panel_x + 10, panel_y + 8))
       
        player = self.get_current_player()
        y = panel_y + 45
       
        for unit_id in available:
            unit_type = UNIT_TYPES[unit_id]
            can_afford = player.stars >= unit_type.cost
           
            # Background
            color = COLORS['panel_light'] if can_afford else (40, 40, 50)
            pygame.draw.rect(self.screen, color, (panel_x + 5, y, panel_w - 10, 36), border_radius=4)
           
            # Name and cost
            txt_color = COLORS['text'] if can_afford else COLORS['text_dim']
            txt = self.font_small.render(f"{unit_type.name} ({unit_type.cost}⭐)", True, txt_color)
            self.screen.blit(txt, (panel_x + 12, y + 4))
           
            # Stats
            stats = f"ATK:{unit_type.atk} DEF:{unit_type.defense} HP:{unit_type.hp}"
            txt = self.font_tiny.render(stats, True, COLORS['text_dim'])
            self.screen.blit(txt, (panel_x + 12, y + 22))
           
            y += 40
   
    def render_particles(self):
        """Render particle effects."""
        for p in self.particles:
            alpha = int(255 * (p.life / 0.8))
            color = (*p.color[:3], alpha) if len(p.color) == 3 else p.color
            pygame.draw.circle(self.screen, p.color, (int(p.x), int(p.y)), int(p.size))
   
    def render_floating_text(self):
        """Render floating text effects."""
        for ft in self.floating_texts:
            alpha = int(255 * ft.timer)
            txt = self.font_small.render(ft.text, True, ft.color)
            self.screen.blit(txt, (ft.x - txt.get_width() // 2, ft.y))
   
    def render_game_over(self):
        """Render game over screen."""
        # Overlay
        s = pygame.Surface((WINDOW_WIDTH, WINDOW_HEIGHT), pygame.SRCALPHA)
        s.fill((0, 0, 0, 180))
        self.screen.blit(s, (0, 0))
       
        # Winner text
        winner = self.players[self.winner]
        txt = self.font_large.render("VICTORY!", True, COLORS['gold'])
        self.screen.blit(txt, (WINDOW_WIDTH // 2 - txt.get_width() // 2, WINDOW_HEIGHT // 2 - 80))
       
        txt = self.font_medium.render(f"{winner.name} Wins!", True, winner.color)
        self.screen.blit(txt, (WINDOW_WIDTH // 2 - txt.get_width() // 2, WINDOW_HEIGHT // 2 - 20))
       
        # Scores
        txt = self.font_small.render(f"Final Scores - Blue: {self.players[0].score}  Red: {self.players[1].score}", True, COLORS['text'])
        self.screen.blit(txt, (WINDOW_WIDTH // 2 - txt.get_width() // 2, WINDOW_HEIGHT // 2 + 30))
       
        # Restart hint
        txt = self.font_small.render("Click anywhere to play again", True, COLORS['text_dim'])
        self.screen.blit(txt, (WINDOW_WIDTH // 2 - txt.get_width() // 2, WINDOW_HEIGHT // 2 + 80))
   
    # =========================================================================
    # UPDATE LOOP
    # =========================================================================
   
    def update(self, dt: float):
        """Update game state."""
        # Update camera shake
        if self.camera_shake > 0:
            self.camera_shake = max(0, self.camera_shake - dt * 30)
       
        # Update message timer
        if self.message_timer > 0:
            self.message_timer -= dt
       
        # Update floating texts
        for ft in self.floating_texts[:]:
            ft.timer -= dt
            ft.y += ft.vel_y * dt
            if ft.timer <= 0:
                self.floating_texts.remove(ft)
       
        # Update particles
        for p in self.particles[:]:
            p.x += p.vel_x * dt
            p.y += p.vel_y * dt
            p.vel_y += 200 * dt  # Gravity
            p.life -= dt
            p.size *= 0.95
            if p.life <= 0:
                self.particles.remove(p)
       
        # Smooth unit animation
        for unit in self.units:
            unit.anim_x += (unit.x - unit.anim_x) * dt * 12
            unit.anim_y += (unit.y - unit.anim_y) * dt * 12
   
    # =========================================================================
    # MAIN LOOP
    # =========================================================================
   
    def run(self):
        """Main game loop."""
        while self.running:
            dt = self.clock.tick(FPS) / 1000.0
           
            # Event handling
            for event in pygame.event.get():
                if event.type == pygame.QUIT:
                    self.running = False
               
                elif event.type == pygame.MOUSEBUTTONDOWN:
                    self.handle_click(event.pos, event.button)
               
                elif event.type == pygame.KEYDOWN:
                    if event.key == pygame.K_ESCAPE:
                        self.running = False
                    elif event.key == pygame.K_SPACE:
                        if self.phase == GamePhase.PLAYING and not self.get_current_player().is_ai:
                            self.end_turn()
                    elif event.key == pygame.K_t:
                        self.show_tech_panel = not self.show_tech_panel
                        self.show_train_panel = False
                    elif event.key == pygame.K_r:
                        self.new_game()
           
            self.update(dt)
            self.render()
       
        pygame.quit()


# =============================================================================
# ENTRY POINT
# =============================================================================

if __name__ == "__main__":
    game = Game()
    game.run()