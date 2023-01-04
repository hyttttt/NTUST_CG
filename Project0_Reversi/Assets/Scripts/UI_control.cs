using System.Collections;
using System.Collections.Generic;
using UnityEngine;
using UnityEngine.SceneManagement;

public class UI_control : MonoBehaviour
{
    SpriteRenderer sprite_rederer;
    public Sprite[] sprite_texture = new Sprite[3];

    // Start is called before the first frame update
    void Start()
    {
        if (gameObject.name == "winner")
        {
            gameObject.transform.position = new Vector3(-100, 0, 0);
        }

        GameObject.Find("background_music").GetComponent<AudioSource>().Stop();
    }

    // Update is called once per frame
    void Update()
    {
        if(gameObject.name == "winner" && Game_manager.winner != 0)
        {
            sprite_rederer = gameObject.GetComponent<SpriteRenderer>();
            sprite_rederer.sprite = sprite_texture[Game_manager.winner % 3];
            gameObject.transform.position = new Vector3(0, 0, 0);
        }
    }

    private void OnMouseUp()
    {
        if(gameObject.name == "music_button")
        {
            sprite_rederer = gameObject.GetComponent<SpriteRenderer>();

            if (sprite_rederer.sprite == sprite_texture[0])
                sprite_rederer.sprite = sprite_texture[1];
            else
                sprite_rederer.sprite = sprite_texture[0];

            Game_manager.music = !Game_manager.music;
            Debug.Log("music" + Game_manager.music);

            AudioSource bgMusicAudioSource = GameObject.Find("background_music").GetComponent<AudioSource>();
            if (Game_manager.music)
            {
                bgMusicAudioSource.Play();
            }
            else
            {
                bgMusicAudioSource.Stop();
            }
        }
        else if (gameObject.name == "hint_button")
        {
            sprite_rederer = gameObject.GetComponent<SpriteRenderer>();

            if (sprite_rederer.sprite == sprite_texture[0])
                sprite_rederer.sprite = sprite_texture[1];
            else
                sprite_rederer.sprite = sprite_texture[0];

            Game_manager.hint = !Game_manager.hint;
            Debug.Log("hint" + Game_manager.hint);
        }
        else if(gameObject.name == "new_game_button")
        {
            SceneManager.LoadScene("GameScene");
        }
    }
}
