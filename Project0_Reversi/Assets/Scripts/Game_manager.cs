using System.Collections;
using System.Collections.Generic;
using UnityEngine;

public class Game_manager : MonoBehaviour
{
    // disk
    const int NONE = 0;
    const int BLACK = 1;
    const int WHITE = 2;

    //dir
    const int UP = 0;
    const int DOWN = 1;
    const int LEFT = 2;
    const int RIGHT = 3;
    const int CROSS_UP_LEFT = 4;
    const int CROSS_UP_RIGHT = 5;
    const int CROSS_DOWN_LEFT = 6;
    const int CROSS_DOWN_RIGHT = 7;

    // game mode
    const int END = 0;
    const int GAME = 1;


    int player;
    int mode;
    public static bool music;
    public static bool hint;
    public static int winner;
    public Number[] nums;
    public Number num_prefab;
    public Disk_obj[] disks_objs;
    public Disk_obj disk_prefab;
    Dictionary<int, int> disk_count;
    List<Vector2Int>[] valid_pos;
    Dictionary<Vector2Int, List<Vector2Int>>[] between_disks;

    void AddDisk(int color, int x, int y)
    {
        int index = y * 8 + x;

        disks_objs[index] = Instantiate(disk_prefab, ScreenPosition(new Vector2Int(x, y)), Quaternion.identity);
        disks_objs[index].color = color;
        disks_objs[index].disk_pos = new Vector2Int(x, y);
    }

    Vector2 ScreenPosition(Vector2Int grid)
    {
        Vector2 center00 = new Vector2(-3.9f, 3.9f);
        float width = 1.12f;
        float height = 1.12f;

        float x = center00.x + grid.x * width;
        float y = center00.y - grid.y * height;

        return new Vector2(x, y);
    }

    bool FindByDir(int x, int y, int color, int dir)
    {
        int i = 0;
        int j = 0;
        int i_dir = 0;
        int j_dir = 0;

        if (dir == UP && y != 0)
        {
            i = x;
            j = y - 1;
            i_dir = 0;
            j_dir = -1;            
        }
        else if (dir == DOWN && y != 7)
        {
            i = x;
            j = y + 1;
            i_dir = 0;
            j_dir = 1;
        }
        else if (dir == LEFT && x != 0)
        {
            i = x - 1;
            j = y;
            i_dir = -1;
            j_dir = 0;
        }
        else if (dir == RIGHT && x != 7)
        {
            i = x + 1;
            j = y;
            i_dir = 1;
            j_dir = 0;
        }
        else if (dir == CROSS_UP_LEFT && x != 0 && y != 0)
        {
            i = x - 1;
            j = y - 1;
            i_dir = -1;
            j_dir = -1;
        }
        else if (dir == CROSS_UP_RIGHT && x != 7 && y != 0)
        {
            i = x + 1;
            j = y - 1;
            i_dir = 1;
            j_dir = -1;
        }
        else if (dir == CROSS_DOWN_LEFT && x != 0 && y != 7)
        {
            i = x - 1;
            j = y + 1;
            i_dir = -1;
            j_dir = 1;
        }
        else if (dir == CROSS_DOWN_RIGHT && x != 7 && y != 7)
        {
            i = x + 1;
            j = y + 1;
            i_dir = 1;
            j_dir = 1;
        }
        else
        {
            return false;
        }

        List<Vector2Int> temp = new List<Vector2Int>();
        while (i >= 0 && i <= 7 &&
                j >= 0 && j <= 7)
        {
            // oppisite color
            if (disks_objs[j * 8 + i].color == (color % 2 + 1))
            {
                temp.Add(new Vector2Int(i, j));
            }
            // same color
            else if (disks_objs[j * 8 + i].color == color)
            {
                break;
            }
            // empty space
            else
            {
                if (temp.Count != 0)
                {
                    // add valid pos
                    Vector2Int pos = new Vector2Int(i, j);
                    valid_pos[color].Add(pos);

                    // add between pos
                    for (int k = 0; k < temp.Count; k++)
                    {
                        if (between_disks[color].ContainsKey(pos))
                        {
                            between_disks[color][pos].Add(temp[k]);
                        }
                        else
                        {
                            between_disks[color].Add(pos, new List<Vector2Int>());
                            between_disks[color][pos].Add(temp[k]);
                        }
                    }
                }

                break;
            }

            i += i_dir;
            j += j_dir;
        }

        return true;
    }

    void FindValidPos()
    {
        valid_pos[WHITE] = new List<Vector2Int>();
        valid_pos[BLACK] = new List<Vector2Int>();
        between_disks[WHITE] = new Dictionary<Vector2Int, List<Vector2Int>>();
        between_disks[BLACK] = new Dictionary<Vector2Int, List<Vector2Int>>();

        for (int y = 0; y < 8; y++)
        {
            for(int x = 0; x < 8; x++)
            {
                int index = y * 8 + x;

                if(disks_objs[index].color != NONE)
                {
                    for(int i = 0; i < 8; i++)
                    {
                        FindByDir(x, y, disks_objs[index].color, UP + i);
                        //Debug.Log("COLOR: " + disks_objs[index].color + " POS: " + x + ", " + y + " DIR: " + (UP + i));
                    }
                }
            }
        }
    }

    bool IsValidPos(int color, Vector2Int mouse)
    {
        bool isValid = false;

        for(int i = 0; i < valid_pos[color].Count; i++)
        {
            if (valid_pos[color][i] == mouse)
            {
                isValid = true;
                break;
            }            
        }

        return isValid;
    }

    void ClickBoard(Vector2Int pos)
    {
        // add new disk, when point to a valid place
        if (valid_pos[player].Count != 0 && IsValidPos(player, pos))
        {
            // add disk
            disks_objs[pos.y * 8 + pos.x].color = player;
            disk_count[player]++;
            disk_count[NONE]--;

            List<Vector2Int> between = between_disks[player][pos];
            for (int i = 0; i < between.Count; i++)
            {
                int index = between[i].y * 8 + between[i].x;

                disks_objs[index].GetComponent<Animator>().SetTrigger("flip");
                disks_objs[index].color = player;
                disk_count[player]++;
                disk_count[player % 2 + 1]--;
            }

            // take turn
            player = player % 2 + 1;
            disk_prefab.GetComponent<Animator>().SetTrigger("flip");
        }
        // take turn, when there is no valid place for current player
        else if (valid_pos[player].Count == 0 && valid_pos[player % 2 + 1].Count != 0)
        {
            player = player % 2 + 1;
            disk_prefab.GetComponent<Animator>().SetTrigger("flip");
        }
    }

    void UpdateScore()
    {
        nums[0].num = disk_count[BLACK] / 10;
        nums[1].num = disk_count[BLACK] % 10;
        nums[2].num = disk_count[WHITE] / 10;
        nums[3].num = disk_count[WHITE] % 10;
    }

    void ShowHint()
    {
        HideHint();

        for (int i = 0; i < valid_pos[player].Count; i++)
        {
            int index = valid_pos[player][i].y * 8 + valid_pos[player][i].x;
            disks_objs[index].GetComponent<Animator>().SetBool("show_hint", true);
        }        
    }

    void HideHint()
    {
        for(int i = 0; i < 64; i++)
        {
            disks_objs[i].GetComponent<Animator>().SetBool("show_hint", false);
        }
    }

    // Start is called before the first frame update
    void Start()
    {
        player = BLACK;
        disk_prefab.color = BLACK;
        mode = GAME;
        music = false;
        hint = false;
        winner = 0;

        disk_count = new Dictionary<int, int>();
        disk_count[NONE] = 60;
        disk_count[BLACK] = 2;
        disk_count[WHITE] = 2;

        disks_objs = new Disk_obj[64];
        for(int y = 0; y < 8; y++)
        {
            for(int x = 0; x < 8; x++)
            {
                if ((x == 3 && y == 3) || (x == 4 && y == 4))
                {
                    AddDisk(WHITE, x, y);
                }
                else if((x == 4 && y == 3) || (x == 3 && y == 4))
                {
                    AddDisk(BLACK, x, y);
                }
                else
                {
                    AddDisk(NONE, x, y);
                }                
            }
        }

        valid_pos = new List<Vector2Int>[3];
        valid_pos[0] = new List<Vector2Int>();
        valid_pos[1] = new List<Vector2Int>();
        valid_pos[2] = new List<Vector2Int>();
        between_disks = new Dictionary<Vector2Int, List<Vector2Int>>[3];

        nums = new Number[4];
        nums[0] = Instantiate(num_prefab, new Vector2(-7.0f, 2.7f), Quaternion.identity);
        nums[1] = Instantiate(num_prefab, new Vector2(-6.4f, 2.7f), Quaternion.identity);
        nums[2] = Instantiate(num_prefab, new Vector2(-7.0f, 1.5f), Quaternion.identity);
        nums[3] = Instantiate(num_prefab, new Vector2(-6.4f, 1.5f), Quaternion.identity);
        UpdateScore();
    }

    // Update is called once per frame
    void Update()
    {
        if (mode == GAME)
        {
        start_again:
            FindValidPos();
            UpdateScore();

            // detect board clicked            
            for (int x = 0; x < 8; x++)
            {
                for (int y = 0; y < 8; y++)
                {
                    if (disks_objs[y * 8 + x].clicked)
                    {
                        ClickBoard(new Vector2Int(x, y));
                        disks_objs[y * 8 + x].clicked = false;

                        goto start_again;
                    }
                }
            }

            // show or hide hint
            if (hint)
                ShowHint();
            else
                HideHint();

            // end game, when there is no valid place for both players
            if (valid_pos[player].Count == 0 && valid_pos[player % 2 + 1].Count == 0)
            {
                mode = END;
            }            
        }
        else if (mode == END)
        {
            // check winner
            if (disk_count[BLACK] > disk_count[WHITE])
            {
                winner = BLACK;
            }
            else if(disk_count[BLACK] < disk_count[WHITE])
            {
                winner = WHITE;
            }
            else
            {
                winner = 3; // tie
            }
        }
    }
}
