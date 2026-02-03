#ifndef SHOP_HPP
#define SHOP_HPP

// Shop state
extern int shop_is_active;

// Upgrade levels
extern int oxygen_upgrade_level;
extern int pick_upgrade_level;
extern int sanity_upgrade_level;
extern int torch_upgrade_level;

// Rock currency
extern int rocks_mined;

// Mining duration (ADD THIS LINE)
extern float rock_mining_duration;

// Shop functions
void Shop_Load();
void Shop_Initialize();
void Shop_Update();
void Shop_Draw();
void Shop_Free();
void Shop_Unload();

// Upgrade functions
void PickUpgrade();
void OxygenUpgrade();
void SanityUpgrade();
void TorchUpgrade();

// Cost functions
int GetPickUpgradeCost();
int GetOxygenUpgradeCost();
int GetSanityUpgradeCost();
int GetTorchUpgradeCost();

#endif // SHOP_HPP
