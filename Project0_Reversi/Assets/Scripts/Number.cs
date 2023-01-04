using System.Collections;
using System.Collections.Generic;
using UnityEngine;

public class Number : MonoBehaviour
{
    SpriteRenderer sprite_rederer;
    public Sprite[] sprite_texture = new Sprite[10];
    public int num;

    // Start is called before the first frame update
    void Start()
    {
        num = 0;
    }

    // Update is called once per frame
    void Update()
    {
        GetComponent<SpriteRenderer>().sprite = sprite_texture[num];
    }
}
