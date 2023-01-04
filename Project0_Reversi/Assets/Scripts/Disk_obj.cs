using UnityEngine;
using UnityEngine.Events;
using UnityEngine.EventSystems;

public class Disk_obj : MonoBehaviour
{
    public int color;     // 1 for black, 2 for white
    public bool clicked;
    public Vector2Int disk_pos;

    private void Start()
    {
        GetComponent<Animator>().SetInteger("color", color);
        clicked = false;
    }

    private void Update()
    {
        GetComponent<Animator>().SetInteger("color", color);
    }

    private void OnMouseUp()
    {
        clicked = true;
        //Debug.Log("click" + disk_pos.x + ", " + disk_pos.y);
    }
}